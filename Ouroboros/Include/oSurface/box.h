// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A 3D shape in texel units used during advanced copies on surfaces
// Dimensions are inclusive on the left/top/front, exclusive on the 
// right/bottom/back. i.e. when left == right, that's a noop.

#pragma once
#include <cstdint>

namespace ouro { namespace surface {

struct box_t
{
	box_t()
		: left(0), right(0)
		, top(0), bottom(1)
		, front(0), back(1)
	{}

	box_t(uint32_t left, uint32_t right, uint32_t top, uint32_t bottom, uint32_t front = 0, uint32_t back = 1) 
    : left(left), right(right), top(top), bottom(bottom), front(front), back(back) {}

	// Convenience when a box is required for 1-dimensional data such as vertex and index buffers
	box_t(uint32_t width) : left(0), right(width), top(0), bottom(1), front(0), back(1) {}

	inline bool empty() const { return left == right || top == bottom || front == back; }
	inline uint32_t width() const { return right - left; }
	inline uint32_t height() const { return bottom - top; }

	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;
	uint32_t front;
	uint32_t back;
};

}}
