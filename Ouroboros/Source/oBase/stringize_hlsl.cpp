// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This cpp contains implemenations of to_string and from_string for intrinsic
// types as well as ouro types.

#include <oMath/hlsl.h>
#include <oString/stringize.h>

namespace ouro {

template<> bool from_string(float2* out_value, const char* src)
{
	return from_string_float_array((float*)out_value, 2, src);
}

template<> bool from_string(float3* out_value, const char* src)
{
	return from_string_float_array((float*)out_value, 3, src);
}

template<> bool from_string(float4* out_value, const char* src)
{
	return from_string_float_array((float*)out_value, 4, src);
}

template<> bool from_string(float4x4* out_value, const char* src)
{
	// Read in-order, then transpose
	bool result = from_string_float_array((float*)out_value, 16, src);
	if (result)
		transpose(*out_value);
	return result;
}

template<> bool from_string(double4x4* out_value, const char* src)
{
	// Read in-order, then transpose
	bool result = from_string_double_array((double*)out_value, 16, src);
	if (result)
		transpose(*out_value);
	return result;
}

#define CHK_MV() do \
	{	if (!out_value || !src) return false; \
		src += strcspn(src, oDIGIT_SIGNED); \
		if (!*src) return false; \
	} while (false)

#define CHK_MV_U() do \
	{	if (!out_value || !src) return false; \
		src += strcspn(src, oDIGIT_UNSIGNED); \
		if (!*src) return false; \
	} while (false)

template<> bool from_string(int2* out_value, const char* src) { CHK_MV(); return 2 == sscanf_s(src, "%d %d", &out_value->x, &out_value->y); }
template<> bool from_string(int3* out_value, const char* src) { CHK_MV(); return 3 == sscanf_s(src, "%d %d %d", &out_value->x, &out_value->y, &out_value->z); }
template<> bool from_string(int4* out_value, const char* src) { CHK_MV(); return 4 == sscanf_s(src, "%d %d %d %d", &out_value->x, &out_value->y, &out_value->z, &out_value->w); }
template<> bool from_string(uint2* out_value, const char* src) { CHK_MV_U(); return 2 == sscanf_s(src, "%u %u", &out_value->x, &out_value->y); }
template<> bool from_string(uint3* out_value, const char* src) { CHK_MV_U(); return 3 == sscanf_s(src, "%u %u %u", &out_value->x, &out_value->y, &out_value->z); }
template<> bool from_string(uint4* out_value, const char* src) { CHK_MV(); return 4 == sscanf_s(src, "%u %u %u %u", &out_value->x, &out_value->y, &out_value->z, &out_value->w); }

template<> char* to_string(char* dst, size_t dst_size, const float2& value) { return -1 != snprintf(dst, dst_size, "%f %f", value.x, value.y) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const float3& value) { return -1 != snprintf(dst, dst_size, "%f %f %f", value.x, value.y, value.z) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const float4& value) { return -1 != snprintf(dst, dst_size, "%f %f %f %f", value.x, value.y, value.z, value.w) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const double2& value) { return -1 != snprintf(dst, dst_size, "%f %f", value.x, value.y) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const double3& value) { return -1 != snprintf(dst, dst_size, "%f %f %f", value.x, value.y, value.z) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const double4& value) { return -1 != snprintf(dst, dst_size, "%f %f %f %f", value.x, value.y, value.z, value.w) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const int2& value) { return -1 != snprintf(dst, dst_size, "%d %d", value.x, value.y) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const int3& value) { return -1 != snprintf(dst, dst_size, "%d %d %d", value.x, value.y, value.z) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const int4& value) { return -1 != snprintf(dst, dst_size, "%d %d %d %d", value.x, value.y, value.z, value.w) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const uint2& value) { return -1 != snprintf(dst, dst_size, "%u %u", value.x, value.y) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const uint3& value) { return -1 != snprintf(dst, dst_size, "%u %u %u", value.x, value.y, value.z) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const uint4& value) { return -1 != snprintf(dst, dst_size, "%u %u %u %u", value.x, value.y, value.z, value.w) ? dst : nullptr; }

template<typename T> char* to_stringT(char* dst, size_t dst_size, const oHLSL4x4<T>& value)
{
	return -1 != snprintf(dst, dst_size, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f"
		, value[0].x, value[1].x, value[2].x, value[3].x
		, value[0].y, value[1].y, value[2].y, value[3].y
		, value[0].z, value[1].z, value[2].z, value[3].z
		, value[0].w, value[1].w, value[2].w, value[3].w) ? dst : nullptr;
}

template<> char* to_string(char* dst, size_t dst_size, const float4x4& value) { return to_stringT(dst, dst_size, value); }
template<> char* to_string(char* dst, size_t dst_size, const double4x4& value) { return to_stringT(dst, dst_size, value); }

}
