// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Hexahedrons: polyhedra with 6 sizes such as cubes, boxes and frusta
// are used freqently. For consistency vertex and face order is 
// consistent with this order throughout Ouroboros code.

#ifndef oHLSL
	#pragma once
#endif

#define oCUBE_LEFT_TOP_NEAR     0
#define oCUBE_LEFT_TOP_FAR      1
#define oCUBE_LEFT_BOTTOM_NEAR  2
#define oCUBE_LEFT_BOTTOM_FAR   3
#define oCUBE_RIGHT_TOP_NEAR    4
#define oCUBE_RIGHT_TOP_FAR     5
#define oCUBE_RIGHT_BOTTOM_NEAR 6
#define oCUBE_RIGHT_BOTTOM_FAR  7
#define oCUBE_CORNER_COUNT      8

// @tony: reorder this so it's consistent with D3D cubemap face order.
#define oCUBE_LEFT              0
#define oCUBE_RIGHT             1
#define oCUBE_TOP               2
#define oCUBE_BOTTOM            3
#define oCUBE_NEAR              4
#define oCUBE_FAR               5
#define oCUBE_FACE_COUNT        6

#define oCUBE_EDGE_COUNT        12
