// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This provides an API similar to that for touch-based devices to abstract 
// skeleton-tracking input devices such as Kinect.

#pragma once
#include <array>
#include <functional>
#include <oMath/hlsl.h>

namespace ouro { namespace windows { namespace skeleton {

// Handle used in window messages to identify a particular skeleton
typedef void* handle;

struct tracking_clipping
{
	tracking_clipping()
		: left(false)
		, right(false)
		, top(false)
		, bottom(false)
		, front(false)
		, back(false)
	{}

	bool left : 1;
	bool right : 1;
	bool top : 1;
	bool bottom : 1;
	bool front : 1;
	bool back : 1;
};

struct bone_info
{
	static const int max_num_bones = 32;

	bone_info(unsigned int _SourceID = 0)
		: source_id(_SourceID)
	{ positions.fill(float4(0.0f, 0.0f, 0.0f, -1.0f)); }

	unsigned int source_id;
	tracking_clipping clipping;
	std::array<float4, max_num_bones> positions;
};

// Use these to register a skeleton source, i.e. Kinect.
// Basically all that's needed is an indirect to call into the integration code
// to get a snapshot of the skeleton.
void register_source(handle _hSkeleton, const std::function<void(bone_info* _pSkeleton)>& _GetSkeleton);
void unregister_source(handle _hSkeleton);

// Call this from an oWM_SKELETON message. Returns false if the results in 
// _pSkeleton are not valid.
bool get_info(handle _hSkeleton, bone_info* _pSkeleton);

}}}
