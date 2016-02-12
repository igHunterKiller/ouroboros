// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// cpu architecture-dependent definitions

#pragma once

#if defined(_M_X64) || defined(_M_AMD64) || defined(__amd64__)
  #define o64BIT 1
  #define o32BIT 0
  #define oCACHE_LINE_SIZE 64
	#define oHAS_DOUBLE_WIDE_ATOMIC_BUG 0
#elif defined(_M_I86) || defined(_M_IX86) || defined(__i386__) || defined(_X86_)
  #define o64BIT 0
  #define o32BIT 1
  #define oCACHE_LINE_SIZE 64
	#define oHAS_DOUBLE_WIDE_ATOMIC_BUG 1
#else
	#error unsupported architecture
#endif
