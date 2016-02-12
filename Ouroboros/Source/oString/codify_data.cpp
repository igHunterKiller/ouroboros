// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/string.h>
#include <iterator>

int snprintf(char* dst, size_t dst_size, const char* fmt, ...);

using namespace std;

namespace ouro {

errno_t replace(char* oRESTRICT result, size_t result_size, const char* oRESTRICT src, const char* find, const char* replace);

template<typename T> struct StaticArrayTraits {};
template<> struct StaticArrayTraits<unsigned char>
{
	static const size_t WORDS_PER_LINE = 20;
	static inline const char* GetFormat() { return "0x%02x,"; }
	static inline const char* GetType() { return "unsigned char"; }
};
template<> struct StaticArrayTraits<unsigned short>
{
	static const size_t WORDS_PER_LINE = 16;
	static inline const char* GetFormat() { return "0x%04x,"; }
	static inline const char* GetType() { return "unsigned short"; }
};
template<> struct StaticArrayTraits<unsigned int>
{
	static const size_t WORDS_PER_LINE = 10;
	static inline const char* GetFormat() { return "0x%08x,"; }
	static inline const char* GetType() { return "unsigned int"; }
};
template<> struct StaticArrayTraits<uint64_t>
{
	static const size_t WORDS_PER_LINE = 10;
	static inline const char* GetFormat() { return "0x%016llx,"; }
	static inline const char* GetType() { return "uint64_t"; }
};

static const char* file_base(const char* path)
{
	const char* p = path + strlen(path) - 1;
	while (p >= path)
	{
		if (*p == '/' || *p == '\\')
			return p + 1;
		p--;
	}
	return nullptr;
}

static char* codify_buffer_name(char* dst, size_t dst_size, const char* path)
{
	if (replace(dst, dst_size, file_base(path), ".", "_"))
		return nullptr;
	return dst;
}

template<size_t size> inline char* codify_buffer_name(char (&dst)[size], const char* path) { return codify_buffer_name(dst, size, path); }

template<typename T>
static size_t codify_data(char* dst, size_t dst_size, const char* buffer_name, const T* buf, size_t buf_size)
{
	const T* words = static_cast<const T*>(buf);
	char* str = dst;
	char* end = str + dst_size - 1; // -1 for terminator
	const size_t nWords = buf_size / sizeof(T);

	str += snprintf(str, dst_size, "const %s sBuffer[] = \n{ // *** AUTO-GENERATED BUFFER, DO NOT EDIT ***", StaticArrayTraits<T>::GetType());
	for (size_t i = 0; i < nWords; i++)
	{
		size_t numberOfElementsLeft = std::distance(str, end);
		if ((i % StaticArrayTraits<T>::WORDS_PER_LINE) == 0 && numberOfElementsLeft > 2)
		{
			*str++ = '\n';
			*str++ = '\t';
			numberOfElementsLeft -= 2;
		}

		str += snprintf(str, numberOfElementsLeft, StaticArrayTraits<T>::GetFormat(), *words++);
	}

	// handle any remaining bytes
	const size_t nExtraBytes = buf_size % sizeof(T);
	if (nExtraBytes)
	{
		uint64_t tmp = 0;
		memcpy(&tmp, &reinterpret_cast<const unsigned char*>(buf)[sizeof(T) * nWords], nExtraBytes);
		str += snprintf(str, std::distance(str, end), StaticArrayTraits<T>::GetFormat(), static_cast<T>(tmp));
	}

	str += snprintf(str, std::distance(str, end), "\n};\n");

	// add accessor function

	char bufferId[_MAX_PATH];
	codify_buffer_name(bufferId, buffer_name);

	uint64_t sz = buf_size; // explicitly size this out so printf formatting below can remain the same between 32- and 64-bit
	str += snprintf(str, std::distance(str, end), "void get_%s(const char** ppBufferName, const void** ppBuffer, size_t* pSize) { *ppBufferName = \"%s\"; *ppBuffer = sBuffer; *pSize = %llu; }\n", bufferId, file_base(buffer_name), sz);

	if (str < end)
		*str++ = 0;

	return std::distance(dst, str);
}

size_t codify_data(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT buffer_name, const void* oRESTRICT buf, size_t buf_size, size_t word_size)
{
	switch (word_size)
	{
		case sizeof(unsigned char): return codify_data(dst, dst_size, buffer_name, static_cast<const unsigned char*>(buf), buf_size);
		case sizeof(unsigned short): return codify_data(dst, dst_size, buffer_name, static_cast<const unsigned short*>(buf), buf_size);
		case sizeof(unsigned int): return codify_data(dst, dst_size, buffer_name, static_cast<const unsigned int*>(buf), buf_size);
		case sizeof(uint64_t): return codify_data(dst, dst_size, buffer_name, static_cast<const uint64_t*>(buf), buf_size);
		default: break;
	}

	return 0;
}

}
