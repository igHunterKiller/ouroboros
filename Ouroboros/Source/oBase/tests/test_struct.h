// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// This is a complex struct useful for reflection/serialization tests because it
// exacerbates internal alignment and requires non-trivial consideration of 
// type usage.
#pragma once
#ifndef oBasisTestStruct_h
#define oBasisTestStruct_h

#include <oBase/date.h>

struct test_struct
{
	bool blA;
	char c1;
	char String[64 * 3];
	bool blB;
	int int1;
	long long long1;
	float floatArray[3];
	int int2;
	bool blC;
	bool blD;
	const char* p1;
	char TestString[64 * 2];
	float float1;
	char FinalTestString[64];
	const char* p2;
	int int3;
	char c2;
	// Expected padding
	int b1size;
	const void* b1;
	const char* NullString;
	int int4;
	int intArray[4];
	double double1;
	float float2;
	int b2size;
	const void* b2;
	ouro::ntp_timestamp time;
};

#define oBASIS_TEST_STRUCT_MEMBERS(_Macro) \
	_Macro(blA) \
	_Macro(c1) \
	_Macro(String) \
	_Macro(blB) \
	_Macro(int1) \
	_Macro(long1) \
	_Macro(floatArray) \
	_Macro(int2) \
	_Macro(blC) \
	_Macro(blD) \
	_Macro(p1) \
	_Macro(TestString) \
	_Macro(float1) \
	_Macro(FinalTestString) \
	_Macro(p2) \
	_Macro(int3) \
	_Macro(c2) \
	_Macro(b1size) \
	_Macro(b1) \
	_Macro(NullString) \
	_Macro(int4) \
	_Macro(intArray) \
	_Macro(double1) \
	_Macro(float2) \
	_Macro(b2size) \
	_Macro(b2) \
	_Macro(time)

void init_test_struct(test_struct* test, const void* buffer1, int buffer1_size, const void* buffer2, int buffer2_size);

bool operator==(const test_struct& x, const test_struct& y);
inline bool operator!=(const test_struct& x, const test_struct& y) { return !(x == y); }

#endif
