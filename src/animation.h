#pragma once

#include <km_common/km_container.h>
#include <km_common/vulkan/km_vulkan_text.h>

const uint64 ANIMATION_MAX_FRAMES = 32;
const uint64 SPRITE_MAX_ANIMATIONS = 8;
const uint64 ANIMATION_QUEUE_MAX_LENGTH = 4;

struct Animation
{
    int fps;
    bool loop;
    uint32 numFrames;
    uint32 frameSpriteIndices[ANIMATION_MAX_FRAMES];
    int frameTiming[ANIMATION_MAX_FRAMES];
    float32 frameTime[ANIMATION_MAX_FRAMES];
    HashTable<uint32> frameExitTo[ANIMATION_MAX_FRAMES];
    bool rootMotion;
    bool rootFollow;
    bool rootFollowEndLoop;
    Vec2 frameRootMotion[ANIMATION_MAX_FRAMES];
    Vec2 frameRootAnchor[ANIMATION_MAX_FRAMES];
};

struct AnimatedSprite
{
    HashTable<Animation> animations;
    HashKey startAnimationKey;
    Vec2Int textureSize;
};

struct AnimatedSpriteInstance
{
    AnimatedSprite* animatedSprite;

    HashKey activeAnimationKey;
    uint32 activeFrame;
    uint32 activeFrameRepeat;
    float32 activeFrameTime;
};

Vec2 UpdateAnimatedSprite(AnimatedSpriteInstance* sprite, float32 deltaTime, const Array<HashKey>& nextAnimations);

#if 0
void DrawAnimatedSprite(const AnimatedSpriteInstance& sprite, Vec2 pos, Vec2 size, Vec2 anchor, Quat rot, float32 alpha, bool flipHorizontal);
#endif

template <uint32 S>
bool LoadAnimatedSprite(const_string name, float32 pixelsPerUnit, LinearAllocator* allocator,
                        VkDevice device, VkPhysicalDevice physicalDevice, VkQueue queue, VkCommandPool commandPool,
                        VulkanSpritePipeline<S>* spritePipeline, AnimatedSprite* sprite);
void UnloadAnimatedSprite(AnimatedSprite* sprite);
