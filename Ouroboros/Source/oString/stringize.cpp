// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/stringize.h>
#include <oString/atof.h>
#include <oString/string.h>
#include <oString/string_fast_scan.h>

oDEFINE_WHITESPACE_PARSING();

namespace ouro {

template<> const char* as_string(const bool& value) { return value ? "true" : "false"; }

template<> char* to_string(char* dst, size_t dst_size, const char* const& value)
{
	return strlcpy(dst, value ? value : "", dst_size) < dst_size ? dst : nullptr;
}

template<> char* to_string(char* dst, size_t dst_size, const char& value)
{
	if (dst_size < 2) return nullptr;
	dst[0] = value;
	dst[1] = 0;
	return dst;
}

template<> char* to_string(char* dst, size_t dst_size, const unsigned char& value)
{
	if (dst_size < 2) return nullptr;
	dst[0] = *(signed char*)&value;
	dst[1] = 0;
	return dst;
}

template<> char* to_string(char* dst, size_t dst_size, const bool& value) { return strlcpy(dst, value ? "true" : "false", dst_size) < dst_size ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const short & value) { return 0 == _itoa_s(value, dst, dst_size, 10) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const unsigned short& value) { return -1 != snprintf(dst, dst_size, "%hu", value) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const int& value) { return 0 == _itoa_s(value, dst, dst_size, 10) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const unsigned int& value) { return -1 != snprintf(dst, dst_size, "%u", value) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const long& value) { return 0 == _itoa_s(value, dst, dst_size, 10) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const unsigned long& value) { return -1 != snprintf(dst, dst_size, "%u", value) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const long long& value) { return 0 == _i64toa_s(value, dst, dst_size, 10) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const uint64_t& value) { return 0 == _ui64toa_s(uint64_t(value), dst, dst_size, 10) ? dst : nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const float& value) { if (-1 != snprintf(dst, dst_size, "%f", value)) { trim_right(dst, dst_size, dst, "0"); return dst; } return nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const double& value) { if (-1 != snprintf(dst, dst_size, "%lf", value)) { trim_right(dst, dst_size, dst, "0"); return dst; } return nullptr; }
template<> char* to_string(char* dst, size_t dst_size, const std::string& value) { return strlcpy(dst, value.c_str(), dst_size) < dst_size ? dst : nullptr; }

template<> bool from_string(bool* out_value, const char* src)
{
	if (!_stricmp("true", src) || !_stricmp("t", src) || !_stricmp("yes", src) || !_stricmp("y", src))
		*out_value = true;
	else 
		*out_value = atoi(src) != 0;
	return true;
}

bool from_string(char* dst, size_t dst_size, const char* src)
{
	return strlcpy(dst, src, dst_size) < dst_size;
}

template<> bool from_string<char*>(char** out_value, const char* src) { return strlcpy(*out_value, src, SIZE_MAX) < SIZE_MAX; }
template<> bool from_string<char>(char* out_value, const char* src) { *out_value = *src; return true; }
template<> bool from_string<unsigned char>(unsigned char* out_value, const char* src) { *out_value = *(const unsigned char*)src; return true; }
template<typename T> inline bool _from_string_int(T* out_value, const char* fmt, const char* src) { return 1 == sscanf_s(src, fmt, out_value); }
template<> bool from_string<short>(short* out_value, const char* src) { return _from_string_int(out_value, "%hd", src); }
template<> bool from_string<unsigned short>(unsigned short* out_value, const char* src) { return _from_string_int(out_value, "%hu", src); }
template<> bool from_string<int>(int* out_value, const char* src) { return _from_string_int(out_value, "%d", src); }
template<> bool from_string<unsigned int>(unsigned int* out_value, const char* src) { return _from_string_int(out_value, "%u", src); }
template<> bool from_string<long>(long* out_value, const char* src) { return _from_string_int(out_value, "%d", src); }
template<> bool from_string<unsigned long>(unsigned long* out_value, const char* src) { return _from_string_int(out_value, "%u", src); }
template<> bool from_string<long long>(long long* out_value, const char* src) { return _from_string_int(out_value, "%lld", src); }
template<> bool from_string<unsigned long long>(unsigned long long* out_value, const char* src) { return _from_string_int(out_value, "%llu", src); }
template<> bool from_string<float>(float* out_value, const char* src) { return atof(src, out_value); }
template<> bool from_string<double>(double* out_value, const char* src) { return _from_string_int(out_value, "%lf", src); }

bool from_string_float_array(float* out_value, size_t num_values, const char* src)
{
	if (!out_value || !src) return false;
	move_past_line_whitespace(&src);
	while (num_values--)
	{
		if (!*src) return false;
		if (!atof(&src, out_value++)) return false;
	}
	return true;
}

bool from_string_double_array(double* out_value, size_t num_values, const char* src)
{
	if (!out_value || !src) return false;
	while (num_values--)
	{
		move_past_line_whitespace(&src);
		if (!*src) return false;
		if (1 != sscanf_s(src, "%lf", out_value)) return false;
		move_to_whitespace(&src);
	}
	return true;
}

}
