// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// snprintf for std::string

#pragma once
#include <oCore/snprintf.h>
#include <string>

namespace ouro {

inline std::string vstringf(const char* format, va_list args)
{
	int n = ouro::vsnprintf(nullptr, 0, format, args);
	std::string str;
	str.resize(n + 1);
	return 0 > ouro::vsnprintf((char*)str.c_str(), str.capacity(), format, args) ? "" : str;
}

inline std::string stringf(const char* format, ...) { va_list args; va_start(args, format); std::string s = vstringf(format, args); va_end(args); return s; }

}
