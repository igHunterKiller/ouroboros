// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Typical structs for vertex arrays

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_vertex_layouts_h
#define oGfx_vertex_layouts_h

#ifndef oHLSL
  #include <oMath/hlsl.h>
	#include <oMesh/mesh.h>
#endif

#include <oMath/hlsl.h>

// _____________________________________________________________________________
// Semantic abstraction

#ifdef oHLSL
	#define oSEM_VS_POSITION   : SV_Position
	#define oSEM_POSITION(idx) : POSITION##idx
	#define oSEM_NORMAL(idx)   : NORMAL##idx
	#define oSEM_TANGENT(idx)  : TANGENT##idx
	#define oSEM_TEXCOORD(idx) : TEXCOORD##idx
	#define oSEM_COLOR(idx)    : COLOR##idx

	#define oVL_SV_POSITION float4 SSposition oSEM_VS_POSITION
	#define oVL_POSITION    float3 position   oSEM_POSITION(0)
	#define oVL_NORMAL      float3 normal     oSEM_NORMAL(0)
	#define oVL_TANGENT     float4 tangent    oSEM_TANGENT(0)
	#define oVL_TEXCOORD20  float2 texcoord0  oSEM_TEXCOORD(0)
	#define oVL_TEXCOORD30  float3 texcoord0  oSEM_TEXCOORD(0)
	#define oVL_TEXCOORD40  float4 texcoord0  oSEM_TEXCOORD(0)
	#define oVL_TEXCOORD21  float2 texcoord1  oSEM_TEXCOORD(0)
	#define oVL_TEXCOORD31  float3 texcoord1  oSEM_TEXCOORD(0)
	#define oVL_TEXCOORD41  float4 texcoord1  oSEM_TEXCOORD(0)
	#define oVL_COLOR0      float4 color0     oSEM_COLOR(0)

	#define oVS_COMPRESSED_POSITION   ushort4 position
	#define oVS_COMPRESSED_NORMAL     uint    normal
	#define oVS_COMPRESSED_TANGENT    uint    tangent
	#define oVS_COMPRESSED_TEXCOORD20 uint    texcoord0
	#define oVS_COMPRESSED_TEXCOORD40 uint2   texcoord0

#else
	#define oVL_SV_POSITION
	#define oVL_POSITION   float3  position
	#define oVL_NORMAL     float3  normal
	#define oVL_TANGENT    float4  tangent
	#define oVL_TEXCOORD20 float2  texcoord0
	#define oVL_TEXCOORD30 float3  texcoord0
	#define oVL_TEXCOORD40 float4  texcoord0
	#define oVL_TEXCOORD21 float2  texcoord1
	#define oVL_TEXCOORD31 float3  texcoord1
	#define oVL_TEXCOORD41 float4  texcoord1
	#define oVL_COLOR0     uint32_t color

	#define oVS_COMPRESSED_POSITION   ushort4 position
	#define oVS_COMPRESSED_NORMAL     dec3n   normal
	#define oVS_COMPRESSED_TANGENT    dec3n   tangent
	#define oVS_COMPRESSED_TEXCOORD20 half2   texcoord0
	#define oVS_COMPRESSED_TEXCOORD40 half4   texcoord0

namespace ouro { namespace gfx {
#endif

// _____________________________________________________________________________
// C++/HLSL structs representing a vertex in one or more vertex arrays

struct VTXpos
{
	oVL_POSITION;
};

struct VTXpos_nrm_tan
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
};

struct VTXpos_uv0
{
	oVL_POSITION;
	oVL_TEXCOORD20;
};

struct VTXpos_uvw
{
	oVL_POSITION;
	oVL_TEXCOORD30;
};

struct VTXpos_col
{
	oVL_POSITION;
	oVL_COLOR0;
};

struct VTXpos_uv0_col
{
	oVL_POSITION;
	oVL_TEXCOORD20;
	oVL_COLOR0;
};

struct VTXpos_nrm_tan_uv0
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
	oVL_TEXCOORD20;
};

struct VTXpos_nrm_tan_uvw
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
	oVL_TEXCOORD30;
};

struct VTXpos_nrm_tan_col
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
	oVL_COLOR0;
};

struct VTXpos_nrm_tan_uv0_uv1
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
	oVL_TEXCOORD20;
	oVL_TEXCOORD21;
};

struct VTXpos_nrm_tan_uv0_col
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
	oVL_TEXCOORD20;
	oVL_COLOR0;
};

struct VTXpos_nrm_tan_uvw_col
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
	oVL_TEXCOORD30;
	oVL_COLOR0;
};

struct VTXpos_nrm_tan_uv0_uv1_col
{
	oVL_POSITION;
	oVL_NORMAL;
	oVL_TANGENT;
	oVL_TEXCOORD20;
	oVL_TEXCOORD21;
	oVL_COLOR0;
};

struct VTXuv0
{
	oVL_TEXCOORD20;
};

struct VTXuv0_uv1
{
	oVL_TEXCOORD20;
	oVL_TEXCOORD21;
};

struct VTXuvw
{
	oVL_TEXCOORD30;
};

struct VTXcol
{
	oVL_COLOR0;
};

struct VTXuv0_col
{
	oVL_TEXCOORD20;
	oVL_COLOR0;
};

struct VTXuv0_uv1_col
{
	oVL_TEXCOORD20;
	oVL_TEXCOORD21;
	oVL_COLOR0;
};

struct VTXuvw_col
{
	oVL_TEXCOORD30;
	oVL_COLOR0;
};

// _____________________________________________________________________________
// HLSL interpolant structs

#ifdef oHLSL

struct INTpos
{
	oVL_SV_POSITION;
};

struct INTpos_uv0
{
	oVL_SV_POSITION;
	float2 texcoord0 oSEM_TEXCOORD(0);
};

struct INTpos_uvw
{
	oVL_SV_POSITION;
	float3 texcoord0 oSEM_TEXCOORD(0);
};

struct INTpos_col
{
	oVL_SV_POSITION;
	float4 color oSEM_COLOR(0);
};

struct INTcommon
{
	oVL_SV_POSITION;
	float3 LSposition  oSEM_POSITION(1);
	float3 WSposition  oSEM_POSITION(2);
	float3 WSbitangent oSEM_TANGENT(0);
	float3 WStangent   oSEM_TANGENT(1);
	float3 WSnormal    oSEM_NORMAL(0);
	float3 texcoord0   oSEM_TEXCOORD(0);
	float2 texcoord1   oSEM_TEXCOORD(1);
	float4 color       oSEM_COLOR(0);
	uint   instance    : SV_InstanceID;
};

#endif

// _____________________________________________________________________________
// Decompression utilities

// return [0,1023] -> [-1,1]
inline float s10tof32(oIN(uint, n10))
{
	return n10 * 0.001955f - 1.0f;
}

// returns [0,3] -> [-1,1]
inline float s2tof32(oIN(uint, n2))
{
	return n2 * 0.666667f - 1.0f;
}

// returns [0,65535] -> [-1,1]
inline float s16tof32(oIN(uint, n16))
{
	return n16 * 0.000030518f - 1.0f;
}

// decodes dec3n -> [-1,1], including 2-bit component in w
inline float4 decode_vector(oIN(uint, dec3n))
{
	return float4(s10tof32(dec3n >> 22), s10tof32((dec3n>>12)&0x3ff), s10tof32((dec3n>>2)&0x3ff), s2tof32(dec3n&0x3));
}

// 4 uint16_t values [0,65535] -> 4 floats [-1,1] then multiplied by scale
inline float4 decode_position(oIN(uint2, u16x4), oIN(float, scale))
{
	return float4(s16tof32(u16x4.x >> 16), s16tof32(u16x4.x & 0xffff), s16tof32(u16x4.x >> 16), s16tof32(u16x4.y & 0xffff)) * scale;
}

// _____________________________________________________________________________
// Configuration API

#ifndef oHLSL

#endif

#ifndef oHLSL
}}
#endif
#endif
