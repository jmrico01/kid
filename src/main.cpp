#include "main.h"

#include <intrin.h>
#include <stdio.h>

#include <stb_image.h>

#include <km_common/km_array.h>
#include <km_common/km_defines.h>
#include <km_common/km_load_obj.h>
#include <km_common/km_os.h>
#include <km_common/km_string.h>
#include <km_common/app/km_app.h>

#include "load_psd.h"

#define ENABLE_THREADS 1

// Required for platform main
const char* WINDOW_NAME = "kid";
const int WINDOW_START_WIDTH  = 1600;
const int WINDOW_START_HEIGHT = 900;
const uint64 PERMANENT_MEMORY_SIZE = MEGABYTES(1);
const uint64 TRANSIENT_MEMORY_SIZE = MEGABYTES(256);

const char* KID_ANIMATION_IDLE = "idle";
const char* KID_ANIMATION_WALK = "walk";
const char* KID_ANIMATION_JUMP = "jump";
const char* KID_ANIMATION_FALL = "fall";
const char* KID_ANIMATION_LAND = "land";

internal AppState* GetAppState(AppMemory* memory)
{
    DEBUG_ASSERT(sizeof(AppState) < memory->permanent.size);
    return (AppState*)memory->permanent.data;
}

internal TransientState* GetTransientState(AppMemory* memory)
{
    DEBUG_ASSERT(sizeof(TransientState) < memory->transient.size);
    TransientState* transientState = (TransientState*)memory->transient.data;
    transientState->scratch = {
        .size = memory->transient.size - sizeof(TransientState),
        .data = memory->transient.data + sizeof(TransientState),
    };
    return transientState;
}

APP_UPDATE_AND_RENDER_FUNCTION(AppUpdateAndRender)
{
    UNREFERENCED_PARAMETER(audio);

    AppState* appState = GetAppState(memory);
    TransientState* transientState = GetTransientState(memory);

    const Vec2Int screenSize = {
        (int)vulkanState.swapchain.extent.width,
        (int)vulkanState.swapchain.extent.height
    };

    // Initialize memory if necessary
    if (!memory->initialized) {
        // TODO this should run on window recreation, fullscreen wipes all sprites
        // Sprites
        {
            const char* spriteFilePaths[] = {
                "data/sprites/jon.png",
                "data/sprites/rock.png"
            };

            for (uint32 i = 0; i < C_ARRAY_LENGTH(spriteFilePaths); i++) {
                int width, height, channels;
                unsigned char* imageData = stbi_load(spriteFilePaths[i], &width, &height, &channels, 0);
                if (imageData == NULL) {
                    DEBUG_PANIC("Failed to load sprite: %s\n", spriteFilePaths[i]);
                }
                defer(stbi_image_free(imageData));

                VulkanImage sprite;
                if (!LoadVulkanImage(vulkanState.window.device, vulkanState.window.physicalDevice,
                                     vulkanState.window.graphicsQueue, appState->vulkanAppState.commandPool,
                                     width, height, channels, (const uint8*)imageData, &sprite)) {
                    DEBUG_PANIC("Failed to Vulkan image for sprite %s\n", spriteFilePaths[i]);
                }

                uint32 spriteIndex;
                if (!RegisterSprite(vulkanState.window.device, &appState->vulkanAppState.spritePipeline, sprite,
                                    &spriteIndex)) {
                    DEBUG_PANIC("Failed to register sprite %s\n", spriteFilePaths[i]);
                }
                DEBUG_ASSERT(spriteIndex == i);
            }
        }

        // TODO this should run on window recreation, fullscreen wipes all fonts
        // Fonts
        {
            LinearAllocator allocator(transientState->scratch);

            struct FontData {
                const_string filePath;
                uint32 height;
            };
            const FontData fontData[] = {
                { ToString("data/fonts/ocr-a/regular.ttf"), 18 },
                { ToString("data/fonts/ocr-a/regular.ttf"), 24 },
            };

            FT_Library ftLibrary;
            FT_Error error = FT_Init_FreeType(&ftLibrary);
            if (error) {
                DEBUG_PANIC("FreeType init error: %d\n", error);
            }

            for (uint32 i = 0; i < C_ARRAY_LENGTH(fontData); i++) {
                LoadFontFaceResult fontFace;
                if (!LoadFontFace(ftLibrary, fontData[i].filePath, fontData[i].height, &allocator, &fontFace)) {
                    DEBUG_PANIC("Failed to load font face at %.*s\n", fontData[i].filePath.size, fontData[i].filePath.data);
                }

                appState->fontFaces[i].height = fontFace.height;
                appState->fontFaces[i].glyphInfo.FromArray(fontFace.glyphInfo);

                VulkanImage fontAtlas;
                if (!LoadVulkanImage(vulkanState.window.device, vulkanState.window.physicalDevice,
                                     vulkanState.window.graphicsQueue, appState->vulkanAppState.commandPool,
                                     fontFace.atlasWidth, fontFace.atlasHeight, 1, fontFace.atlasData, &fontAtlas)) {
                    DEBUG_PANIC("Failed to Vulkan image for font atlas %lu\n", i);
                }

                uint32 fontIndex;
                if (!RegisterFont(vulkanState.window.device, &appState->vulkanAppState.textPipeline, fontAtlas,
                                  &fontIndex)) {
                    DEBUG_PANIC("Failed to register font %lu\n", i);
                }
                DEBUG_ASSERT(fontIndex == i);
            }
        }

        // other
        {
            LinearAllocator allocator(transientState->scratch);

            if (!LoadAnimatedSprite(ToString("kid"), 10.0f, &allocator,
                                    vulkanState.window.device, vulkanState.window.physicalDevice,
                                    vulkanState.window.graphicsQueue, appState->vulkanAppState.commandPool,
                                    &appState->vulkanAppState.spritePipeline, &appState->animatedSpriteKid)) {
                DEBUG_PANIC("Failed to load kid animated sprite\n");
            }

            appState->animatedSpriteInstanceKid.animatedSprite = &appState->animatedSpriteKid;
            appState->animatedSpriteInstanceKid.activeAnimationKey.WriteString(KID_ANIMATION_IDLE);
            appState->animatedSpriteInstanceKid.activeFrame = 0;
            appState->animatedSpriteInstanceKid.activeFrameRepeat = 0;
            appState->animatedSpriteInstanceKid.activeFrameTime = 0.0f;
        }

        memory->initialized = true;
    }

    // Reset frame state
    {
        ResetSpriteRenderState(&transientState->frameState.spriteRenderState);
        ResetTextRenderState(&transientState->frameState.textRenderState);
    }

    const Array<HashKey> nextAnimations = Array<HashKey>::empty;
    UpdateAnimatedSprite(&appState->animatedSpriteInstanceKid, deltaTime, nextAnimations);

    // DrawAnimatedSprite(appState->animatedSpriteInstanceKid, )

    const uint32 NUM_SPRITES = 10;
    for (uint32 i = 0; i < NUM_SPRITES; i++) {
        const Vec2Int pos = { RandInt(0, screenSize.x), RandInt(0, screenSize.y) };
        const Vec2Int size = { RandInt(50, 100), RandInt(50, 300) };
        PushSprite((uint32)SpriteId::PSD_SPRITES, pos, size, 0.5f, screenSize,
                   &transientState->frameState.spriteRenderState);
    }

    const_string text = ToString("the quick brown fox jumps over the lazy dog");
    const Vec4 textColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    const uint32 fontIndex = (uint32)FontId::OCR_A_REGULAR_18;
    PushText(fontIndex, appState->fontFaces[fontIndex], text, Vec2Int { 100, 100 }, 0.0f, screenSize, textColor,
             appState->vulkanAppState.textPipeline, &transientState->frameState.textRenderState);

    // ================================================================================================
    // Vulkan rendering ===============================================================================
    // ================================================================================================

    VkCommandBuffer buffer = appState->vulkanAppState.commandBuffer;

    // TODO revisit this. should the platform coordinate something like this in some other way?
    // swapchain image acquisition timings seem to be kind of sloppy tbh, so this might be the best way.
    VkFence fence = appState->vulkanAppState.fence;
    if (vkWaitForFences(vulkanState.window.device, 1, &fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        LOG_ERROR("vkWaitForFences didn't return success for fence %lu\n", swapchainImageIndex);
    }
    if (vkResetFences(vulkanState.window.device, 1, &fence) != VK_SUCCESS) {
        LOG_ERROR("vkResetFences didn't return success for fence %lu\n", swapchainImageIndex);
    }

    if (vkResetCommandBuffer(buffer, 0) != VK_SUCCESS) {
        LOG_ERROR("vkResetCommandBuffer failed\n");
    }

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    if (vkBeginCommandBuffer(buffer, &beginInfo) != VK_SUCCESS) {
        LOG_ERROR("vkBeginCommandBuffer failed\n");
    }

    const VkClearValue clearValues[] = {
        { 1.0f, 1.0f, 1.0f, 1.0f },
        { 1.0f, 0 }
    };

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkanState.swapchain.renderPass;
    renderPassInfo.framebuffer = vulkanState.swapchain.framebuffers[swapchainImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = vulkanState.swapchain.extent;
    renderPassInfo.clearValueCount = C_ARRAY_LENGTH(clearValues);
    renderPassInfo.pClearValues = clearValues;

    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Sprites
    {
        LinearAllocator allocator(transientState->scratch);
        UploadAndSubmitSpriteDrawCommands(vulkanState.window.device, buffer, appState->vulkanAppState.spritePipeline,
                                          transientState->frameState.spriteRenderState, &allocator);
    }

    // Text
    {
        LinearAllocator allocator(transientState->scratch);
        UploadAndSubmitTextDrawCommands(vulkanState.window.device, buffer, appState->vulkanAppState.textPipeline,
                                        transientState->frameState.textRenderState, &allocator);
    }

    vkCmdEndRenderPass(buffer);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS) {
        LOG_ERROR("vkEndCommandBuffer failed\n");
    }

    const VkSemaphore waitSemaphores[] = { vulkanState.window.imageAvailableSemaphore };
    const VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    const VkSemaphore signalSemaphores[] = { vulkanState.window.renderFinishedSemaphore };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = C_ARRAY_LENGTH(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;
    submitInfo.signalSemaphoreCount = C_ARRAY_LENGTH(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(vulkanState.window.graphicsQueue, 1, &submitInfo, fence) != VK_SUCCESS) {
        LOG_ERROR("Failed to submit draw command buffer\n");
    }

    return true;
}

APP_LOAD_VULKAN_SWAPCHAIN_STATE_FUNCTION(AppLoadVulkanSwapchainState)
{
    LOG_INFO("Loading Vulkan swapchain-dependent app state\n");

    const VulkanWindow& window = vulkanState.window;
    const VulkanSwapchain& swapchain = vulkanState.swapchain;

    VulkanAppState* app = &(GetAppState(memory)->vulkanAppState);
    TransientState* transientState = GetTransientState(memory);
    LinearAllocator allocator(transientState->scratch);

    if (!LoadSpritePipelineSwapchain(window, swapchain, &allocator, &app->spritePipeline)) {
        LOG_ERROR("Failed to load swapchain-dependent Vulkan sprite pipeline\n");
        return false;
    }

    if (!LoadTextPipelineSwapchain(window, swapchain, &allocator, &app->textPipeline)) {
        LOG_ERROR("Failed to load swapchain-dependent Vulkan text pipeline\n");
        return false;
    }

    return true;
}

APP_UNLOAD_VULKAN_SWAPCHAIN_STATE_FUNCTION(AppUnloadVulkanSwapchainState)
{
    LOG_INFO("Unloading Vulkan swapchain-dependent app state\n");

    const VkDevice& device = vulkanState.window.device;
    VulkanAppState* app = &(GetAppState(memory)->vulkanAppState);

    UnloadTextPipelineSwapchain(device, &app->textPipeline);
    UnloadSpritePipelineSwapchain(device, &app->spritePipeline);
}

APP_LOAD_VULKAN_WINDOW_STATE_FUNCTION(AppLoadVulkanWindowState)
{
    LOG_INFO("Loading Vulkan window-dependent app state\n");

    const VulkanWindow& window = vulkanState.window;

    VulkanAppState* app = &(GetAppState(memory)->vulkanAppState);
    TransientState* transientState = GetTransientState(memory);
    LinearAllocator allocator(transientState->scratch);

    // Create command pool
    {
        QueueFamilyInfo queueFamilyInfo = GetQueueFamilyInfo(window.surface, window.physicalDevice, &allocator);

        VkCommandPoolCreateInfo poolCreateInfo = {};
        poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolCreateInfo.queueFamilyIndex = queueFamilyInfo.graphicsFamilyIndex;
        poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(window.device, &poolCreateInfo, nullptr, &app->commandPool) != VK_SUCCESS) {
            LOG_ERROR("vkCreateCommandPool failed\n");
            return false;
        }
    }

    // Create command buffer
    {
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = app->commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(window.device, &allocInfo, &app->commandBuffer) != VK_SUCCESS) {
            LOG_ERROR("vkAllocateCommandBuffers failed\n");
            return false;
        }
    }

    // Create fence
    {
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateFence(window.device, &fenceCreateInfo, nullptr, &app->fence) != VK_SUCCESS) {
            LOG_ERROR("vkCreateFence failed\n");
            return false;
        }
    }

    const bool spritePipeline = LoadSpritePipelineWindow(window, app->commandPool, &allocator, &app->spritePipeline);
    if (!spritePipeline) {
        LOG_ERROR("Failed to load window-dependent Vulkan sprite pipeline\n");
        return false;
    }

    const bool textPipeline = LoadTextPipelineWindow(window, app->commandPool, &allocator, &app->textPipeline);
    if (!textPipeline) {
        LOG_ERROR("Failed to load window-dependent Vulkan text pipeline\n");
        return false;
    }

    return true;
}

APP_UNLOAD_VULKAN_WINDOW_STATE_FUNCTION(AppUnloadVulkanWindowState)
{
    LOG_INFO("Unloading Vulkan window-dependent app state\n");

    const VkDevice& device = vulkanState.window.device;
    VulkanAppState* app = &(GetAppState(memory)->vulkanAppState);

    UnloadSpritePipelineWindow(device, &app->spritePipeline);
    UnloadTextPipelineWindow(device, &app->textPipeline);

    vkDestroyFence(device, app->fence, nullptr);
    vkDestroyCommandPool(device, app->commandPool, nullptr);
}

#include "animation.cpp"
#include "load_psd.cpp"

#include <km_common/km_array.cpp>
#include <km_common/km_container.cpp>
#include <km_common/km_kmkv.cpp>
#include <km_common/km_load_font.cpp>
#include <km_common/km_load_obj.cpp>
#include <km_common/km_memory.cpp>
#include <km_common/km_os.cpp>
#include <km_common/km_string.cpp>

#include <km_common/app/km_app.cpp>
#include <km_common/app/km_input.cpp>

#include <km_common/vulkan/km_vulkan_core.cpp>
#include <km_common/vulkan/km_vulkan_sprite.cpp>
#include <km_common/vulkan/km_vulkan_text.cpp>
#include <km_common/vulkan/km_vulkan_util.cpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#undef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#undef STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_SPRINTF_IMPLEMENTATION
#include <stb_sprintf.h>
#undef STB_SPRINTF_IMPLEMENTATION
