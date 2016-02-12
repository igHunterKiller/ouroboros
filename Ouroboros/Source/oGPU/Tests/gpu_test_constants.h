// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#ifndef oHLSL
	#pragma once
#endif
#ifndef oGPU_test_constants_h
#define oGPU_test_constants_h

#include <oGfx/oGfxHLSL.h>

#ifndef oHLSL
#include <oMath/quantize.h>
#endif

#define oGPU_TEST_CB_CONSTANTS_SLOT 0

#define oGPU_TEST_SRV_TEXTURE_SLOT 0

struct test_constants
{
#ifndef oHLSL
	test_constants() { set_default(); }
	test_constants(const float4x4& _world, const float4x4& _view, const float4x4& _projection, const float4& _color) { set(_world, _view, _projection, _color); }
	test_constants(const float4x4& _world, const float4x4& _view, const float4x4& _projection, uint32_t _argb) { set(_world, _view, _projection, _argb); }

	inline void set(const float4x4& _world, const float4x4& _view, const float4x4& _projection, const float4& _color)
	{
		world = world;
		wvp = _world * _view * _projection;
		color = _color;
	}

	inline void set(const float4x4& _world, const float4x4& _view, const float4x4& _projection, uint32_t _argb)
	{
		set(_world, _view, _projection, ouro::truetofloat4(_argb));
	}

	inline void set_default()
	{
		 world = kIdentity4x4;
		 wvp = kIdentity4x4;
		 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
	}
#endif

	float4x4 world;
	float4x4 wvp;
	float4   color;
};

#ifdef oHLSL
cbuffer cbuffer_test_constants : register(b0) { test_constants g_test_constants[2]; }

// LS: Local Space
// WS: World Space
// SS: Screen Space
// VS: View Space

struct VSIN
{
	float3 LSposition : POSITION;
	float3 LSnormal   : NORMAL;
	float2 texcoord0  : TEXCOORD;
};

struct VSOUT
{
	float4 SSposition : SV_Position;
	float3 WSposition : POSITION;
	float3 LSposition : POSITION1;
	float3 WSnormal   : NORMAL;
	float3 texcoord0  : TEXCOORD;
	float4 color      : COLOR;
};

struct PSOUT
{
	float4 color;
};

float4 oGPUTestLStoSS(float3 LSposition, uint instance)
{
	return mul(g_test_constants[instance].wvp, float4(LSposition, 1));
}

float3 oGPUTestLStoWS(float3 LSposition, uint instance)
{
	return mul(g_test_constants[instance].world, float4(LSposition, 1)).xyz;
}

float3 oGPUTestRotateLStoWS(float3 LSvector, uint instance)
{
	return mul((float3x3)g_test_constants[instance].world, LSvector);
}

VSOUT CommonVS(VSIN In)
{
	VSOUT Out = (VSOUT)0;
	Out.SSposition = oGPUTestLStoSS(In.LSposition, 0);
	Out.WSposition = oGPUTestLStoWS(In.LSposition, 0);
	Out.LSposition = In.LSposition;
	Out.WSnormal = oGPUTestRotateLStoWS(In.LSnormal, 0);
	Out.texcoord0 = float3(In.texcoord0, 0);
	Out.color = g_test_constants[0].color;
	return Out;
}

VSOUT CommonVS(float3 LSposition, float3 texcoord)
{
	VSOUT Out = (VSOUT)0;
	Out.SSposition = oGPUTestLStoSS(LSposition, 0);
	Out.WSposition = oGPUTestLStoWS(LSposition, 0);
	Out.LSposition = LSposition;
	Out.color = g_test_constants[0].color;
	Out.texcoord0 = texcoord;
	return Out;
}

#endif
#endif
