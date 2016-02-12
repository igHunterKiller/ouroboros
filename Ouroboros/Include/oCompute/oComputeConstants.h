// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// This code contains constants that can be used in either HLSL or C++.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oComputeConstants_h
#define oComputeConstants_h

#include <oMath/hlsl.h>
#include <oMath/hlslx.h>
#include <oMath/floats.h>

// _____________________________________________________________________________
// Other constants

static const float3 oBLACK3 = float3(0.0f, 0.0f, 0.0f);
static const float3 oWHITE3 = float3(1.0f, 1.0f, 1.0f);
static const float3 oRED3 = float3(1.0f,0.0f,0.0f);
static const float3 oGREEN3 = float3(0.0f, 1.0f,0.0f);
static const float3 oBLUE3 = float3(0.0f,0.0f, 1.0f);
static const float3 oYELLOW3 = float3(1.0f, 1.0f,0.0f);
static const float3 oMAGENTA3 = float3(1.0f,0.0f, 1.0f);
static const float3 oCYAN3 = float3(0.0f, 1.0f, 1.0f);

static const float4 oBLACK4 = float4(0.0f, 0.0f, 0.0f, 1.0f);
static const float4 oWHITE4 = float4(1.0f, 1.0f, 1.0f, 1.0f);
static const float4 oRED4 = float4(1.0f, 0.0f, 0.0f, 1.0f);
static const float4 oGREEN4 = float4(0.0f, 1.0f, 0.0f, 1.0f);
static const float4 oBLUE4 = float4(0.0f, 0.0f, 1.0f, 1.0f);
static const float4 oYELLOW4 = float4(1.0f, 1.0f, 0.0f, 1.0f);
static const float4 oMAGENTA4 = float4(1.0f, 0.0f, 1.0f, 1.0f);
static const float4 oCYAN4 = float4(0.0f, 1.0f, 1.0f, 1.0f);

static const float3 oCOLORS3[] = { oBLACK3, oBLUE3, oGREEN3, oCYAN3, oRED3, oMAGENTA3, oYELLOW3, oWHITE3, };
static const float4 oCOLORS4[] = { oBLACK4, oBLUE4, oGREEN4, oCYAN4, oRED4, oMAGENTA4, oYELLOW4, oWHITE4, };

// Three unit length vectors that approximate a hemisphere
// with a normal of float3(0.0f, 0.0f, 1.0f)
static const float3 oHEMISPHERE3[3] = 
{
	float3(0.0f, oTHREE_QUARTERS_SQRT3f, 0.5f ),
	float3(-0.75f, -oTHREE_QUARTERS_SQRT3f * 0.5f, 0.5f),
	float3(0.75f, -oTHREE_QUARTERS_SQRT3f * 0.5f, 0.5f),
};

static const float3 oHEMISPHERE4[4] = 
{
	float3(0.0f, oTHREE_QUARTERS_SQRT3f, 0.5f),
	float3(-oTHREE_QUARTERS_SQRT3f, 0.0f, 0.5f),
	float3(0.0f, -oTHREE_QUARTERS_SQRT3f, 0.5f),
	float3(oTHREE_QUARTERS_SQRT3f, 0.0f, 0.5f),
};

#endif
