// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oArch/compiler.h>
#include <oString/string.h>
#include <cerrno>

namespace ouro {

errno_t replace(char* oRESTRICT result, size_t result_size, const char* oRESTRICT src, char _ChrFind, char replace)
{
	const char* oRESTRICT r = src;
	char* oRESTRICT w = result;
	while (1)
	{
		*w++ = *r == _ChrFind ? replace : *r;
		if (!*r)
			break;
		r++;
	}
	return 0;
}

errno_t replace(char* oRESTRICT result, size_t result_size, const char* oRESTRICT src, const char* oRESTRICT find, const char* oRESTRICT replace)
{
	if (!result || !src) return EINVAL;
	if (result == src) return EINVAL;
	if (!find)
		return strlcpy(result, src, result_size) < result_size ? 0 : ENOBUFS;
	if (!replace)
		replace = "";

	size_t findLen = strlen(find);
	size_t replaceLen = strlen(replace);
	const char* s = strstr(src, find);

	while (s)
	{
		size_t len = s - src;
		#ifdef _MSC_VER
			errno_t e = strncpy_s(result, result_size, src, len);
			if (e)
				return e;
		#else
			strncpy(result, src, len);
		#endif
		result += len;
		result_size -= len;
		if (strlcpy(result, replace, result_size) >= result_size)
			return ENOBUFS;
		result += replaceLen;
		result_size -= replaceLen;
		src += len + findLen;
		s = strstr(src, find);
	}

	// copy the rest
	return strlcpy(result, src, result_size) ? 0 : EINVAL;
}

}
