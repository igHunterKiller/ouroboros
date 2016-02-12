// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// common byte operations

#pragma once
#include <cstddef>
#include <cstdint>

namespace ouro {

template<typename T> T      align     (T value, size_t alignment)                           { return (T)(((size_t)value + alignment - 1) & ~(alignment - 1)); }
template<typename T> T      align_down(T value, size_t alignment)                           { return (T)( (size_t)value & ~(alignment - 1));                  }
template<typename T> bool   aligned   (T value, size_t alignment)                           { return align(value, alignment) == value;                        }
template<typename T> T*     byte_add  (T* ptr, size_t bytes)                                { return (T*)(((char*)ptr) + bytes);                              }
inline               size_t index_of  (const void* el, const void* base, size_t size)       { return size_t((char*)el - (char*)base) / size;                  }
template<typename T> size_t index_of  (const T* el,    const T*    base)                    { return index_of(el, base, sizeof(T));                           }
inline               bool   in_range  (const void* test, const void* base, size_t size    ) { return (test >= base) && (test < ((const char*)base + size));   }
inline               bool   in_range  (const void* test, const void* base, const void* end) { return (test >= base) && (test < end);                          }

// _____________________________________________________________________________
// Swizzles

union byte_swizzle16
{
	int16_t  asint16;
	uint16_t asuint16;
	int8_t   asint8[2];
	uint8_t  asuint8[2];
};

union byte_swizzle32
{
	float         asfloat;
	int32_t       asint32;
	uint32_t      asuint32;
	long          aslong;
	unsigned long asulong;
	int16_t       asint16[2];
	uint16_t      asuint16[2];
	int8_t        asint8[4];
	uint8_t       asuint8[4];
};

union byte_swizzle64
{
	double             asdouble;
	int64_t            asint64;
	uint64_t           asuint64;
	unsigned long long asullong;
	long long          asllong;
	float              asfloat[2];
	int32_t            asint32[2];
	uint32_t           asuint32[2];
	long               aslong[2];
	unsigned long      asulong[2];
	int16_t            asint16[4];
	uint16_t           asuint16[4];
	int8_t             asint8[8];
	uint8_t            asuint8[8];
};

}
