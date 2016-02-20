// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// implement stringize api for std containers

#pragma once
#include <oCore/stringize.h>
#include <oString/string.h>
#include <string>
#include <vector>

namespace ouro {

template<> char* to_string(char* dst, size_t dst_size, const std::string& value) { return strlcpy(dst, value.c_str(), dst_size) < dst_size ? dst : nullptr; }

namespace detail {

template<typename containerT> char* to_string_container(char* dst, size_t dst_size, const containerT& cont)
{
	*dst = 0;
	auto it_last = std::end(cont) - 1;
	for (auto it = std::begin(cont); it != std::end(cont); ++it)
	{
		if (!to_string(dst, dst_size, *it))
			return nullptr;
		size_t len = strlcat(dst, ",", dst_size);
		if (it != it_last && len >= dst_size)
			return nullptr;
		dst += len;
		dst_size -= len;
	}
	return dst;
}

template<typename containerT> bool from_string_container(containerT* out_container, const char* src)
{
	char* ctx = nullptr;
	const char* tok = ouro::strtok(src, ",", &ctx);
	while (tok)
	{
		containerT::value_type obj;
		tok += strspn(tok, oWHITESPACE);
		if (!ouro::from_string(&obj, tok) || out_container->size() == out_container->max_size())
		{
			ouro::end_strtok(&ctx);
			return false;
		}
		out_container->push_back(obj);
		tok = ouro::strtok(nullptr, ",", &ctx);
	}
	return true;
}

}

// _____________________________________________________________________________
// Std container support

template <typename T, typename AllocatorT> char* to_string(char* dst, size_t dst_size, const std::vector<T, AllocatorT>& vec) { return detail::to_string_container(dst, dst_size, vec); }
template <typename T, typename AllocatorT> bool from_string(std::vector<T, AllocatorT>* out_value, const char* src) { return detail::from_string_container(out_value, src); }

}
