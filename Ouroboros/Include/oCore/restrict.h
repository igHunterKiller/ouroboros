// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// restrict keyword

#pragma once
#if defined(_MSC_VER)
  #define oRESTRICT __restrict
#else
	#error Unsupported compiler
#endif
