// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <gpu_test_constants.h>

static int TESTGPUBufferAppendIndices[20] = 
{ 5, 6, 7, 18764, 2452, 2423, 52354, 344, -1542, 3434, 53, -4535, 3535, 88884747, 534535, 88474, -445, 4428855, -1235, 4661};

void VStest_buffer(in uint ID : SV_VertexID, out float4 Position : SV_Position, out int Index : TESTBUFFERINDEX)
{
	Position = float4(0.0f, 0.0f, 0.0f, 1.0f);
	Index = TESTGPUBufferAppendIndices[ID];
}

AppendStructuredBuffer<int> Output : register(u0);

void PStest_buffer(in float4 Position : SV_Position, in int Index : TESTBUFFERINDEX)
{
	Output.Append(Index);
}

float4 PStest_color(VSOUT In) : SV_Target
{
	return In.color;
}

VSOUT VStest_pass_through_color(float3 LSposition : POSITION, float4 color : COLOR)
{
	VSOUT Out = (VSOUT)0;
	Out.SSposition = float4(LSposition,1);
	Out.color = color;
	return Out;
}

VSOUT VStest_color(float3 LSposition : POSITION, float4 color : COLOR, uint instance : SV_InstanceID)
{
	VSOUT Out = (VSOUT)0;
	Out.SSposition = oGPUTestLStoSS(LSposition, instance);
	Out.color = color;
	return Out;
}

VSOUT VStest_pos(float3 LSposition : POSITION, uint instance : SV_InstanceID)
{
	VSOUT Out = (VSOUT)0;
	Out.SSposition = oGPUTestLStoSS(LSposition, instance);
	return Out;
}

VSOUT VStest_texture1d(float3 LSposition : POSITION, float3 texcoord0 : texcoord0)
{
	return CommonVS(LSposition, texcoord0);
}

SamplerState Bilin : register(s0);
Texture1D Diffuse1D : register(t0);

float4 PStest_texture1d(VSOUT In) : SV_Target
{
	return Diffuse1D.Sample(Bilin, In.texcoord0.x);
}

VSOUT VStest_texture2d(float3 LSposition : POSITION, float3 texcoord0 : texcoord0)
{
	return CommonVS(LSposition, texcoord0);
}

float4 PStest_texcoords_as_color(VSOUT In) : SV_Target
{
	// @tony WHAT is going on here? moving x and y to r and g just isn't working

	return float4(0, frac(In.texcoord0.x), frac(In.texcoord0.y), 1);
}

Texture2D Diffuse2D : register(t0);

float4 PStest_texture2d(VSOUT In) : SV_Target
{
	return Diffuse2D.Sample(Bilin, In.texcoord0.xy);
}

VSOUT VStest_texture3d(float3 LSposition : POSITION, float3 texcoord0 : texcoord0)
{
	return CommonVS(LSposition, texcoord0);
}

Texture3D Diffuse3D : register(t0);

float4 PStest_texture3d(VSOUT In) : SV_Target
{
	return Diffuse3D.Sample(Bilin, In.texcoord0);
}

VSOUT VStest_texturecube(float3 LSposition : POSITION, float3 texcoord0 : texcoord0)
{
	return CommonVS(LSposition, LSposition);
}

TextureCube DiffuseCube : register(t0);

float4 PStest_texturecube(VSOUT In) : SV_Target
{
	return DiffuseCube.Sample(Bilin, In.texcoord0);
}
