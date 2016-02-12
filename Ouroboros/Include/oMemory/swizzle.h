// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// swizzle structs for easy type conversions

#pragma once
#include <stddef.h>
#include <cstdint>
#include <oCore/byte.h>

namespace ouro {

union byte_swizzle16
{
	int16_t asint16;
	uint16_t asuint16;
	char asint8[2];
	uint8_t asuint8[2];
};

union byte_swizzle32
{
	float asfloat;
	int32_t asint32;
	uint32_t asuint32;
	long aslong;
	unsigned long asulong;
	int16_t asint16[2];
	uint16_t asuint16[2];
	int8_t asint8[4];
	uint8_t asuint8[4];
};

union byte_swizzle64
{
	double asdouble;
	int64_t asint64;
	uint64_t asuint64;
	unsigned long long asullong;
	long long asllong;
	float asfloat[2];
	int32_t asint32[2];
	uint32_t asuint32[2];
	long aslong[2];
	unsigned long asulong[2];
	int16_t asint16[4];
	uint16_t asuint16[4];
	int8_t asint8[8];
	uint8_t asuint8[8];
};

}
