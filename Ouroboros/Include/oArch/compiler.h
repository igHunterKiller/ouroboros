// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// compiler-dependent definitions

#pragma once

#define oVS2010_VER 1600
#define oVS2012_VER 1700
#define oVS2013_VER 1800
#define oVS2015_VER 1900

#if defined(_MSC_VER)

// _____________________________________________________________________________
#if defined(_WIN64) || defined(WIN64)
	#define oDEFAULT_MEMORY_ALIGNMENT 16
#elif defined(_WIN32) || defined(WIN32)
	#define oDEFAULT_MEMORY_ALIGNMENT 4
#endif

#define oDEBUGBREAK __debugbreak()

#pragma warning(disable:4324) // structure was padded due to _declspec(align())

// _____________________________________________________________________________
#if _MSC_VER == oVS2012_VER || _MSC_VER == oVS2013_VER

  // C++11 support
  #define oALIGNAS(x)   __declspec(align(x))
  #define oALIGNOF(x)   __alignof(x)
  #define oTHREAD_LOCAL __declspec(thread)
  #define oHAS_CBEGIN   0

  // low-level optimization support
  #define oRESTRICT     __restrict
  #define oFORCEINLINE  __forceinline

	#define oHAS_EXP2     0
	#define oHAS_LOG2     0

  // disable warnings that don't seem to be squelched in project settings
  #pragma warning(disable:4481) // nonstandard extension used: override specifier 'override'

// _____________________________________________________________________________
#elif _MSC_VER == oVS2015_VER

  // C++11 support
  #define oALIGNAS(x)   alignas(x)
  #define oALIGNOF(x)   alignof(x)
  #define oTHREAD_LOCAL thread_local
  #define oHAS_CBEGIN   0

  // low-level optimization support
  #define oRESTRICT     __restrict
  #define oFORCEINLINE  __forceinline

	#define oHAS_EXP2     1
	#define oHAS_LOG2     1

// _____________________________________________________________________________
#else
	#error unsupported compiler
#endif

#endif