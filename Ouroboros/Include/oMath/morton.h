// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// morton numbers, specifically as used in octree spatial partitioning

// http://en.wikipedia.org/wiki/Morton_number_(number_theory)
// Morton numbers take n-dimensional integer coordinates to a single dimension 
// by interleaving the bits of each coordinate. The resulting number then 
// contains a series of n-length bit sequences that describe progressive 
// navigation of quadtrees/octrees and so on.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oMath_morton_h
#define oMath_morton_h

#include <oMath/hlsl.h>
#include <oMath/hlslx.h>

// We can store 2^10 values for each component of XYZ in a 32-bit Morton number.
// We assume that because the Morton number exists, it's a valid path into an
// Octree (if it was culled by the octree's root bounds, the number would not be 
// up for consideration in whatever algorithm being implemented). Thus we can 
// store one more level than 2^10. Another way to look at this is that 2^11 is
// 1024 and there can be 1024 values in 10 bits.
static const uint oMORTON_OCTREE_MAX_TARGET_DEPTH = 11;
static const int oMORTON_OCTREE_SCALAR = (1 << (oMORTON_OCTREE_MAX_TARGET_DEPTH - 1)) - 1;
static const float oMORTON_OCTREE_SCALAR_FLOAT = (float)oMORTON_OCTREE_SCALAR;

// 32-bit Morton numbers can only use 30-bits for XYZ, 10 each channel, so 
// define a mask used to get rid of any non-Morton usage of the upper two bits.
#define oMORTON_MASK 0x3fffffff

// x must be 10-bit or less
inline uint spreadbits2(uint x)
{
	// http://stackoverflow.com/questions/1024754/how-to-compute-a-3d-morton-number-interleave-the-bits-of-3-ints?answertab=votes#tab-top
	x = (x | (x << 16)) & 0x030000FF;
	x = (x | (x <<  8)) & 0x0300F00F;
	x = (x | (x <<  4)) & 0x030C30C3;
	x = (x | (x <<  2)) & 0x09249249;
	return x;
}

inline uint morton3d(oIN(uint3, position))
{
	return (spreadbits2(position.x) << oAXIS_X) | (spreadbits2(position.y) << oAXIS_Y) | (spreadbits2(position.z) << oAXIS_Z);
}

//    +---------+ Returns [0,7] as to what local octant the specified point is 
//   / 1  / 0  /| in. Use oAXIS_X/oAXIS_Y/oAXIS_Z to check specific dimensions 
//  /----/----/ | axis.
//  +---+---+/|0|
//  | 5 | 4 | |/| depth_index=0 for the root, which is not represented in the
//  +---+---+/|2| Morton number because it always evaluates to 0 (the root) or 
//  | 7 | 6 | |/  the number is not in the octree (outside the root bounds). So
//  +---+---+/    for the 1st subdivision (8 octants as pictured) where depth=1,
//                This would get the highest 3 bits of the 30-bit Morton number.
inline uint to_octree(uint morton3d, uint depth_index)
{
	return (morton3d >> (30 - depth_index * 3)) & 0x7;
}

// Given a Morton number of arbitrary precision (i.e. one directly calculated
// from any (normalized) float position) truncate values beyond the depth_index 
// such that values at a higher precision evaluate to the same value. Using 
// to_octree with values larger than the one specified here will yield invalid 
// information.
inline uint morton_quantize(uint depth_index)
{
	// Provides 1 to depth_index inclusively. See above docs describing that 0
	// is assumed.
	return oMORTON_MASK - ((1 << (3*(oMORTON_OCTREE_MAX_TARGET_DEPTH-1-depth_index))) - 1);
}

// all position values must be on [0,1]. They will be encoded to [0,1023] and 
// then to a morton3d value
inline uint morton_encode(oIN(float3, normalized_position))
{
	float3 fpos = oMORTON_OCTREE_SCALAR_FLOAT * normalized_position;
	uint3 ipos = uint3((uint)fpos.x, (uint)fpos.y, (uint)fpos.z);
	return morton3d(ipos);
}

// returns the normalized position encoded using morton_encode()
inline float3 morton_decode(uint morton3d, uint target_depth = oMORTON_OCTREE_MAX_TARGET_DEPTH)
{
	float3 pos = 0.0f;
	float offset = 0.5f;
	for(uint i = 1; i < target_depth; ++i)
	{  
		uint oct_select = to_octree(morton3d, i);
		pos.x += (oct_select & (0x1 << oAXIS_X)) ? offset : 0.0f;
		pos.y += (oct_select & (0x1 << oAXIS_Y)) ? offset : 0.0f;
		pos.z += (oct_select & (0x1 << oAXIS_Z)) ? offset : 0.0f;
		offset *= 0.5f;
	}
	pos += offset;
	return pos;
}

#endif
