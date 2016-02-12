// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

void VSfullscreen_tri(uint id : SV_VertexID, out float4 out_SSposition : SV_Position, out float2 out_texcoord0 : TEXCOORD)
{
	out_texcoord0 = float2((id << 1) & 2, id & 2); 
	out_SSposition = float4(out_texcoord0 * float2(2, -2) + float2(-1, 1), 0, 1);
}

void VSpass_through_pos(float3 LSposition : POSITION, out float4 out_SSposition : SV_Position)
{
	out_SSposition = float4(LSposition, 1);
}

float4 PSblack() : SV_Target
{
	return float4(1, 1, 1, 1);
}

float4 PSwhite() : SV_Target
{
	return float4(1, 1, 1, 1);
}

Texture2D tex2d : register(t0);
SamplerState tex2dsampler : register(s0);
float4 PStex2d(float4 SSposition : SV_Position, float2 texcoord0 : TEXCOORD) : SV_Target
{
	return tex2d.Sample(tex2dsampler, texcoord0);
}

[numthreads(1, 1, 1)] void CSNoop()
{
}
