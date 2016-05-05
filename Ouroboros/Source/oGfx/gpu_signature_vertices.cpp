// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oGfx/gpu_signature_vertices.h>
#include <oBase/type_info.h>
#include <oMath/quantize.h>

namespace ouro {

template<> size_t to_string(char* dst, size_t dst_size, const gfx::VTXp& vtx)
{
	size_t offset = 0;
	size_t indent = 0;

	STRUCTF_BEGIN_ACC(dst, dst_size, indent, "VTXp", &vtx);
		indent++;
		FIELDF_ACC(dst, dst_size, indent, vtx.position);
		indent--;
	STRUCTF_END_ACC(dst, dst_size, indent, "VTXp", &vtx);

	return offset;
}

template<> size_t to_string(char* dst, size_t dst_size, const gfx::VTXpc& vtx)
{
	size_t offset = 0;
	size_t indent = 0;

	STRUCTF_BEGIN_ACC(dst, dst_size, indent, "VTXpc", &vtx);
		indent++;
		FIELDF_ACC(dst, dst_size, indent, vtx.position);
		float4 color = truetofloat4(vtx.color);
		FIELDF_ACC(dst, dst_size, indent, color);
		indent--;
	STRUCTF_END_ACC(dst, dst_size, indent, "VTXpc", &vtx);

	return offset;
}

template<> size_t to_string(char* dst, size_t dst_size, const gfx::VTXpnt& vtx)
{
	size_t offset = 0;
	size_t indent = 0;

	STRUCTF_BEGIN_ACC(dst, dst_size, indent, "VTXpnt", &vtx);
		indent++;
		FIELDF_ACC(dst, dst_size, indent, vtx.position);
		FIELDF_ACC(dst, dst_size, indent, vtx.normal);
		FIELDF_ACC(dst, dst_size, indent, vtx.tangent);
		indent--;
	STRUCTF_END_ACC(dst, dst_size, indent, "VTXpnt", &vtx);

	return offset;
}

template<> size_t to_string(char* dst, size_t dst_size, const gfx::VTXpntu& vtx)
{
	size_t offset = 0;
	size_t indent = 0;

	STRUCTF_BEGIN_ACC(dst, dst_size, indent, "VTXpntu", &vtx);
		indent++;
		FIELDF_ACC(dst, dst_size, indent, vtx.position);
		FIELDF_ACC(dst, dst_size, indent, vtx.normal);
		FIELDF_ACC(dst, dst_size, indent, vtx.tangent);
		FIELDF_ACC(dst, dst_size, indent, vtx.texcoord0);
		indent--;
	STRUCTF_END_ACC(dst, dst_size, indent, "VTXpntu", &vtx);

	return offset;
}

template<> size_t to_string(char* dst, size_t dst_size, const gfx::VTXu& vtx)
{
	size_t offset = 0;
	size_t indent = 0;

	STRUCTF_BEGIN_ACC(dst, dst_size, indent, "VTXu", &vtx);
		indent++;
		FIELDF_ACC(dst, dst_size, indent, vtx.texcoord0);
		indent--;
	STRUCTF_END_ACC(dst, dst_size, indent, "VTXu", &vtx);

	return offset;
}

template<> size_t to_string(char* dst, size_t dst_size, const gfx::VTXc& vtx)
{
	size_t offset = 0;
	size_t indent = 0;

	STRUCTF_BEGIN_ACC(dst, dst_size, indent, "VTXc", &vtx);
		indent++;
		FIELDF_ACC(dst, dst_size, indent, vtx.color);
		indent--;
	STRUCTF_END_ACC(dst, dst_size, indent, "VTXc", &vtx);

	return offset;
}

float4 decode_position(oIN(uint2, positions), float position_scale, float curvature_scale)
{
	uint4 pos = uint4(positions.x >> 16, positions.x, positions.y >> 16, positions.y) & 0xffff;
	return s16tof32(pos) * float4(position_scale, position_scale, position_scale, curvature_scale);
}

template<> size_t to_string(char* dst, size_t dst_size, const gfx::VTXmesh& vtx)
{
	size_t offset = 0;
	size_t indent = 0;

	STRUCTF_BEGIN_ACC(dst, dst_size, indent, "VTXmesh", &vtx);
		indent++;
		float4 position   = decode_position(vtx.position, 1.0f, 1.0f);
		float4 quaternion = udec3tofloat4(vtx.quaternion);
		float2 texcoord0  = float2(f16tof32(uint16_t(vtx.texcoord0 >> 16)), f16tof32(uint16_t(vtx.texcoord0)));
		FIELDF_ACC(dst, dst_size, indent, position);
		FIELDF_ACC(dst, dst_size, indent, quaternion);
		FIELDF_ACC(dst, dst_size, indent, texcoord0);
		indent--;
	STRUCTF_END_ACC(dst, dst_size, indent, "VTXmesh", &vtx);

	return offset;
}

namespace gfx {

size_t to_string_VTXp      (char* dst, size_t dst_size, const void* vtx) { return to_string(dst, dst_size, *(const VTXp*)      vtx); }
size_t to_string_VTXpnt    (char* dst, size_t dst_size, const void* vtx) { return to_string(dst, dst_size, *(const VTXpnt*)    vtx); }
size_t to_string_VTXpc     (char* dst, size_t dst_size, const void* vtx) { return to_string(dst, dst_size, *(const VTXpc*)     vtx); }
size_t to_string_VTXpntu   (char* dst, size_t dst_size, const void* vtx) { return to_string(dst, dst_size, *(const VTXpntu*)   vtx); }
size_t to_string_VTXu      (char* dst, size_t dst_size, const void* vtx) { return to_string(dst, dst_size, *(const VTXu*)      vtx); }
size_t to_string_VTXc      (char* dst, size_t dst_size, const void* vtx) { return to_string(dst, dst_size, *(const VTXc*)      vtx); }

}

}