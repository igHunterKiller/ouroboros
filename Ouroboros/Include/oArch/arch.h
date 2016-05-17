// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// cpu architecture-dependent & compiler-dependent definitions

#pragma once

#if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__)
	#define o64BIT                    1
  #define o32BIT                    0
  #define oCACHE_LINE_SIZE          64
	#define oDEFAULT_MEMORY_ALIGNMENT 16
#elif defined(_M_I86) || defined(_M_IX86) || defined(__i386__) || defined(_X86_)
  #define o64BIT                    0
  #define o32BIT                    1
  #define oCACHE_LINE_SIZE          64
	#define oDEFAULT_MEMORY_ALIGNMENT 4
#else
	#error unsupported architecture
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1900 // VS 2015
	#pragma warning(disable:4324) // structure was padded due to _declspec(align())
  #define oRESTRICT __restrict
#else
	#error Unsupported compiler
#endif
