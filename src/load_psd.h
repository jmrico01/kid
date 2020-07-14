#pragma once

#include <km_common/km_array.h>

enum class LayerBlendMode
{
	NORMAL,
	MULTIPLY
};

enum class LayerChannelID
{
	RED   = 0,
	GREEN = 1,
	BLUE  = 2,
	ALPHA = 3,

	ALL
};

struct LayerChannelInfo
{
	LayerChannelID channelID;
	uint32 dataSize;
};

struct ImageData
{
	Vec2Int size;
	uint8 channels;
	uint8* data;
};

struct PsdLayerInfo
{
    static const uint32 MAX_NAME_LENGTH = 256;
    static const uint32 MAX_CHANNELS = 4;

	FixedArray<char, MAX_NAME_LENGTH> name;
	int left, right, top, bottom;
	FixedArray<LayerChannelInfo, MAX_CHANNELS> channels;
	uint8 opacity;
	LayerBlendMode blendMode;
	bool visible;
	uint32 parentIndex;
	bool inTimeline;
	float32 timelineStart;
	float32 timelineDuration;
    uint32 dataStart;
};

struct PsdFile
{
	Vec2Int size;
	Array<PsdLayerInfo> layers;
	Array<uint8> file;

};

bool LoadPsdLayerImageData(const PsdFile& psdFile, uint32 layerIndex, LayerChannelID channel, LinearAllocator* allocator,
                           ImageData* outImageData);

bool LoadPsd(const_string filePath, LinearAllocator* allocator, PsdFile* psdFile);
void FreePsd(LinearAllocator* allocator, PsdFile* psdFile);
