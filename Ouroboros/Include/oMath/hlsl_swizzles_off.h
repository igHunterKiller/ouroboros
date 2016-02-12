// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This header is designed to cross-compile in both C++ and HLSL. This defines
// for C++ the HLSL language feature of swizzling tuple elements. These macros
// can easily pollute client code so take care to guard shader code with 
// hlsl_swizzles_on.h/hlsl_swizzles_off.h sentinels.

#ifndef oHLSL
#pragma pop_macro("xy")
#pragma pop_macro("yx")
#pragma pop_macro("xx")
#pragma pop_macro("yy")
#pragma pop_macro("xyz")
#pragma pop_macro("xzy")
#pragma pop_macro("yxz")
#pragma pop_macro("yzx")
#pragma pop_macro("zxy")
#pragma pop_macro("zyx")
#pragma pop_macro("xxx")
#pragma pop_macro("yyy")
#pragma pop_macro("zzz")
#pragma pop_macro("xz")
#pragma pop_macro("yx")
#pragma pop_macro("yz")
#pragma pop_macro("zx")
#pragma pop_macro("zy")
#pragma pop_macro("xxxx")
#pragma pop_macro("yyyy")
#pragma pop_macro("xyzw")
#pragma pop_macro("wxyz")
#pragma pop_macro("zwxy")
#pragma pop_macro("yzwx")
#pragma pop_macro("xyz")
#endif
