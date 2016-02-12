// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// compiler-dependent definitions

#pragma once

#define oVS2010_VER 1600
#define oVS2012_VER 1700
#define oVS2013_VER 1800

#if defined(_MSC_VER) && _MSC_VER == oVS2012_VER

#if defined(_WIN64) || defined(WIN64)
	#define oDEFAULT_MEMORY_ALIGNMENT 16
#elif defined(_WIN32) || defined(WIN32)
	#define oDEFAULT_MEMORY_ALIGNMENT 4
#endif

// C++11 support
#define oALIGNAS(x) __declspec(align(x))
#define oALIGNOF(x) __alignof(x)
#define oNOEXCEPT
#define oTHREAD_LOCAL __declspec(thread)
#define oHAS_CBEGIN 0

// low-level optimization support
#define oRESTRICT __restrict
#define oFORCEINLINE __forceinline

// disable warnings that don't seem to be squelched in project settings
#pragma warning(disable:4481) // nonstandard extension used: override specifier 'override'
#pragma warning(disable:4324) // structure was padded due to _declspec(align())

#else
	#error unsupported compiler
#endif
