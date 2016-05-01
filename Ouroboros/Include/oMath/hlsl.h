// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// An implementation of the HLSL shader language for C++, including helper 
// macros to aid in cross-compilation of shader code. 
// This header is designed to cross-compile in both C++ and HLSL. There are
// some macros, especially for in, out, inout semantics, that if used will
// do the proper thing in C++ and HLSL version of the code.

#ifndef oMath_hlsl_h
#define oMath_hlsl_h

#ifndef oHLSL

#pragma once

#include <oArch/compiler.h>
#include <oArch/intrin.h>
#include <assert.h>
#include <float.h>
#include <math.h>
#include <memory.h>
#include <type_traits>

#ifndef oHLSL_USE_FAST_ASINT
	#define oHLSL_USE_FAST_ASINT 0
#endif

#ifndef oHLSL_USE_FAST_RCP
	#define oHLSL_USE_FAST_RCP 0
#endif

#ifndef oHLSL_USE_FAST_RSQRT
	#define oHLSL_USE_FAST_RSQRT 0
#endif

#ifndef oHLSL_USE_FAST_LOG2
	#define oHLSL_USE_FAST_LOG2 0
#endif

#ifndef oHLSL_OURO_EQUAL_SUPPORT
	#define oHLSL_OURO_EQUAL_SUPPORT 1
#endif

// _____________________________________________________________________________
// Printf helpers

#define FMT_FLOAT2   "%f %f"
#define FMT_FLOAT3   "%f %f %f"
#define FMT_FLOAT4   "%f %f %f %f"
#define FMT_FLOAT3X3 "%f %f %f|%f %f %f|%f %f %f"
#define FMT_FLOAT4X4 "%f %f %f %f|%f %f %f %f|%f %f %f %f|%f %f %f %f"

#define PRM_FLOAT2(v) (v).x, (v).y
#define PRM_FLOAT3(v) (v).x, (v).y, (v).z
#define PRM_FLOAT4(v) (v).x, (v).y, (v).z, (v).w

#define PRM_FLOAT3X3(m) PRM_FLOAT3((m)[0]), PRM_FLOAT3((m)[1]), PRM_FLOAT3((m)[2])
#define PRM_FLOAT4X4(m) PRM_FLOAT4((m)[0]), PRM_FLOAT4((m)[1]), PRM_FLOAT4((m)[2]), PRM_FLOAT4((m)[3])

// _____________________________________________________________________________
// Assignment macros used to define tuples

#define oHLSL_ASSIGN(r, p, op) inline const r& operator op##=(const p& a) { *this = *this op a; return *this; }
#define oHLSL_ASSIGNS(r, p) const p& operator[](size_t i) const { return *(&x + i); } p& operator[](size_t i) { return *(&x + i); } \
	oHLSL_ASSIGN(r,p,<<) oHLSL_ASSIGN(r,r,<<) oHLSL_ASSIGN(r,p,>>) oHLSL_ASSIGN(r,r,>>) \
  oHLSL_ASSIGN(r,p, +) oHLSL_ASSIGN(r,r, +) oHLSL_ASSIGN(r,p, -) oHLSL_ASSIGN(r,r, -) \
  oHLSL_ASSIGN(r,p, *) oHLSL_ASSIGN(r,r, *) oHLSL_ASSIGN(r,p, /) oHLSL_ASSIGN(r,r, /) oHLSL_ASSIGN(r,p, %) oHLSL_ASSIGN(r,r, %) \
  oHLSL_ASSIGN(r,p, &) oHLSL_ASSIGN(r,r, &) oHLSL_ASSIGN(r,p, |) oHLSL_ASSIGN(r,r, |) oHLSL_ASSIGN(r,p, ^) oHLSL_ASSIGN(r,r, ^)

// _____________________________________________________________________________
// Operator macros used to define tuples

// Define the a op b core constructors
#define oHLSL_VOP2(op,r,a1,a2,b1,b2)             { return oHLSL2<r>(a1 op b1, a2 op b2);                     }
#define oHLSL_VOP3(op,r,a1,a2,a3,b1,b2,b3)       { return oHLSL3<r>(a1 op b1, a2 op b2, a3 op b3);           }
#define oHLSL_VOP4(op,r,a1,a2,a3,a4,b1,b2,b3,b4) { return oHLSL4<r>(a1 op b1, a2 op b2, a3 op b3, a4 op b4); }

// Define 2,3,4 vector, scalar; scalar, vector; and vector, vector operations
#define oHLSL_VV2(op,r)  template<typename T> oHLSL2<r> operator op(const oHLSL2<T>& a, const oHLSL2<T>& b)       oHLSL_VOP2(op,T,a.x,a.y,b.x,b.y)
#define oHLSL_VV3(op,r)  template<typename T> oHLSL3<r> operator op(const oHLSL3<T>& a, const oHLSL3<T>& b)       oHLSL_VOP3(op,T,a.x,a.y,a.z,b.x,b.y,b.z)
#define oHLSL_VV4(op,r)  template<typename T> oHLSL4<r> operator op(const oHLSL4<T>& a, const oHLSL4<T>& b)       oHLSL_VOP4(op,T,a.x,a.y,a.z,a.w,b.x,b.y,b.z,b.w)
#define oHLSL_VS2(op,p)  template<typename T> oHLSL2<T> operator op(const oHLSL2<T>& a, const p&         b)       oHLSL_VOP2(op,T,a.x,a.y,b,b)
#define oHLSL_VS3(op,p)  template<typename T> oHLSL3<T> operator op(const oHLSL3<T>& a, const p&         b)       oHLSL_VOP3(op,T,a.x,a.y,a.z,b,b,b)
#define oHLSL_VS4(op,p)  template<typename T> oHLSL4<T> operator op(const oHLSL4<T>& a, const p&         b)       oHLSL_VOP4(op,T,a.x,a.y,a.z,a.w,b,b,b,b)
#define oHLSL_SV2(op)    template<typename T> oHLSL2<T> operator op(const T&         a, const oHLSL2<T>& b)       oHLSL_VOP2(op,T,a,a,b.x,b.y)
#define oHLSL_SV3(op)	   template<typename T> oHLSL3<T> operator op(const T&         a, const oHLSL3<T>& b)       oHLSL_VOP3(op,T,a,a,a,b.x,b.y,b.z)
#define oHLSL_SV4(op)	   template<typename T> oHLSL4<T> operator op(const T&         a, const oHLSL4<T>& b)       oHLSL_VOP4(op,T,a,a,a,a,b.x,b.y,b.z,b.w)
#define oHLSL_MVV2(op,r)                      oHLSL2<r> operator op(                    const oHLSL2<T>& b) const oHLSL_VOP2(op,r,x,y,b.x,b.y)
#define oHLSL_MVV3(op,r)                      oHLSL3<r> operator op(                    const oHLSL3<T>& b) const oHLSL_VOP3(op,r,x,y,z,b.x,b.y,b.z)
#define oHLSL_MVV4(op,r)                      oHLSL4<r> operator op(                    const oHLSL4<T>& b) const oHLSL_VOP4(op,r,x,y,z,w,b.x,b.y,b.z,b.w)

// Reduce the above definitions to just 2,3,4 permutations
#define oHLSL_OP2(op) oHLSL_VV2(op,T) oHLSL_VS2(op,T) oHLSL_SV2(op)
#define oHLSL_OP3(op) oHLSL_VV3(op,T) oHLSL_VS3(op,T) oHLSL_SV3(op)
#define oHLSL_OP4(op) oHLSL_VV4(op,T) oHLSL_VS4(op,T) oHLSL_SV4(op)

// Select based on element count and define operators for the global namespace & methods
#define oHLSL_OPS(n) oHLSL_OP##n(*) oHLSL_OP##n(/) oHLSL_OP##n(+) oHLSL_OP##n(-) oHLSL_OP##n(%) oHLSL_VS##n(<<,int) oHLSL_VS##n(>>,int) oHLSL_VS##n(&,int) oHLSL_VS##n(|,int) oHLSL_VS##n(^,int) oHLSL_VV##n(!=,bool) oHLSL_VV##n(==,bool) oHLSL_VV##n(<,bool) oHLSL_VV##n(>,bool) oHLSL_VV##n(<=,bool) oHLSL_VV##n(>=,bool)
#define oHLSL_MOPS(n) oHLSL_MVV##n(+, T) oHLSL_MVV##n(-, T) oHLSL_MVV##n(*, T) oHLSL_MVV##n(/, T) oHLSL_MVV##n(%, T) oHLSL_MVV##n(==, bool) oHLSL_MVV##n(!=, bool) oHLSL_MVV##n(<, bool) oHLSL_MVV##n(>, bool) oHLSL_MVV##n(<=, bool) oHLSL_MVV##n(>=, bool)

// _____________________________________________________________________________
// Swizzle operations. These are still functions as defined here (requires parens())
// See hlsl_swizzles_on/off.h for non-parens extensions.

#define oHLSL_SW2(a,b)     inline const oHLSL2<T> a##b()       const { return oHLSL2<T>(a,b);     }
#define oHLSL_SW3(a,b,c)   inline const oHLSL3<T> a##b##c()    const { return oHLSL3<T>(a,b,c);   }
#define oHLSL_SW4(a,b,c,d) inline const oHLSL4<T> a##b##c##d() const { return oHLSL4<T>(a,b,c,d); }

#define oHLSL_SWIZZLE2(type)        oHLSL_SW2(x,x)   oHLSL_SW2(x,y)   oHLSL_SW2(y,x)   oHLSL_SW2(y,y)
#define oHLSL_SWIZZLE3(type)        oHLSL_SW2(y,z)   oHLSL_SW2(z,y)   oHLSL_SW2(x,z)   oHLSL_SW2(z,x)   oHLSL_SW2(z,z)   oHLSL_SWIZZLE2(type) \
	oHLSL_SW3(x,x,x) oHLSL_SW3(x,x,y) oHLSL_SW3(x,x,z) oHLSL_SW3(x,y,x) oHLSL_SW3(x,y,y) oHLSL_SW3(x,y,z) oHLSL_SW3(x,z,x) \
	oHLSL_SW3(x,z,y) oHLSL_SW3(x,z,z) oHLSL_SW3(y,x,x) oHLSL_SW3(y,x,y) oHLSL_SW3(y,x,z) oHLSL_SW3(y,y,x) oHLSL_SW3(y,y,y) \
	oHLSL_SW3(y,y,z) oHLSL_SW3(y,z,x) oHLSL_SW3(y,z,y) oHLSL_SW3(y,z,z) oHLSL_SW3(z,x,x) oHLSL_SW3(z,x,y) oHLSL_SW3(z,x,z) \
	oHLSL_SW3(z,y,x) oHLSL_SW3(z,y,y) oHLSL_SW3(z,y,z) oHLSL_SW3(z,z,x) oHLSL_SW3(z,z,y) oHLSL_SW3(z,z,z)
#define oHLSL_SWIZZLE4(type) oHLSL_SWIZZLE3(type) oHLSL_SW2(z,w) oHLSL_SW2(w,z) oHLSL_SW2(x,w) oHLSL_SW2(w,x) oHLSL_SW2(y,w) oHLSL_SW2(w,y) oHLSL_SW2(w,w) \
	oHLSL_SW3(w,w,w) oHLSL_SW3(y,z,w) oHLSL_SW3(z,w,x) oHLSL_SW3(w,z,y) oHLSL_SW3(y,x,w) \
	oHLSL_SW4(x,y,z,w) oHLSL_SW4(x,x,x,x) oHLSL_SW4(y,y,y,y) oHLSL_SW4(z,z,z,z) oHLSL_SW4(w,w,w,w)
// todo: Add the rest of the swizzle3 with W permutations and swizzle4 permutations

// Tie assigment, operators and swizzles into a single macro
#define oHLSL_MEMBERS(n) oHLSL_SWIZZLE##n(T) oHLSL_MOPS(n) oHLSL_ASSIGNS(oHLSL##n<T>, T)

// _____________________________________________________________________________
// Tuplize functions

// Functions that take two params also have different stdc names than hlsl
#define oHLSL_FN22(hlslfn, stdfn) template<typename T> oHLSL2<T> hlslfn(const oHLSL2<T>& a, const oHLSL2<T>& b) { return oHLSL2<T>(stdfn(a.x, b.x), stdfn(a.y, b.y)); }
#define oHLSL_FN23(hlslfn, stdfn) template<typename T> oHLSL3<T> hlslfn(const oHLSL3<T>& a, const oHLSL3<T>& b) { return oHLSL3<T>(stdfn(a.x, b.x), stdfn(a.y, b.y), stdfn(a.z, b.z)); }
#define oHLSL_FN24(hlslfn, stdfn) template<typename T> oHLSL4<T> hlslfn(const oHLSL4<T>& a, const oHLSL4<T>& b) { return oHLSL4<T>(stdfn(a.x, b.x), stdfn(a.y, b.y), stdfn(a.z, b.z), stdfn(a.w, b.w)); }

// Functions that take one param are consistent with hlsl
#define oHLSL_FN12(fn) template<typename T> oHLSL2<T> fn(const oHLSL2<T>& a) { return oHLSL2<T>(fn(a.x), fn(a.y)); }
#define oHLSL_FN13(fn) template<typename T> oHLSL3<T> fn(const oHLSL3<T>& a) { return oHLSL3<T>(fn(a.x), fn(a.y), fn(a.z)); }
#define oHLSL_FN14(fn) template<typename T> oHLSL4<T> fn(const oHLSL4<T>& a) { return oHLSL4<T>(fn(a.x), fn(a.y), fn(a.z), fn(a.w)); }

// Define all tupple version per function
#define oHLSL_FN1S(fn) oHLSL_FN12(fn) oHLSL_FN13(fn) oHLSL_FN14(fn)
#define oHLSL_FN2S(hlslfn, stdfn) oHLSL_FN22(hlslfn, stdfn) oHLSL_FN23(hlslfn, stdfn) oHLSL_FN24(hlslfn, stdfn)

// _____________________________________________________________________________
// Matrix definitions

#define oHLSL_MAT_BRACKET_OP(r) const r& operator[](size_t i) const { return *(&col0 + i); } r& operator[](size_t i) { return *(&col0 + i); }

// _____________________________________________________________________________
// Basic tuples

template<typename T> struct oHLSL2
{
	T x,y;
	typedef T element_type;
	inline                        oHLSL2()                      noexcept                                                        {}
	inline                        oHLSL2(const oHLSL2& that)    noexcept : x(that.x),                 y(that.y)                 {}
	inline                        oHLSL2(T xy)                  noexcept : x(xy),                     y(xy)                     {}
	inline                        oHLSL2(T _x, T _y)            noexcept : x(_x),                     y(_y)                     {}
	template<typename U>          oHLSL2(const oHLSL2<U>& that) noexcept : x(static_cast<T>(that.x)), y(static_cast<T>(that.y)) {}
	template<typename U> explicit oHLSL2(U xy)                  noexcept : x(static_cast<T>(xy)),     y(static_cast<T>(xy))     {}
	oHLSL2 operator-() const { return oHLSL2(-x, -y); }
	oHLSL_MEMBERS(2)
};

template<typename T> struct oHLSL3
{
	T x,y,z;
	typedef T element_type;
	inline                        oHLSL3()                          noexcept                                                                                   {}
	inline                        oHLSL3(const oHLSL3& that)        noexcept : x(that.x),                 y(that.y),                 z(that.z)                 {}
	inline                        oHLSL3(T xyz)                     noexcept : x(xyz),                    y(xyz),                    z(xyz)                    {}
	inline                        oHLSL3(T _x, T _y, T _z)          noexcept : x(_x),                     y(_y),                     z(_z)                     {}
	inline                        oHLSL3(const oHLSL2<T>& xy, T _z) noexcept : x(xy.x),                   y(xy.y),                   z(_z)                     {}
	template<typename U>          oHLSL3(const oHLSL3<U>& that)     noexcept : x(static_cast<T>(that.x)), y(static_cast<T>(that.y)), z(static_cast<T>(that.z)) {}
	template<typename U> explicit oHLSL3(U xyz)                     noexcept : x(static_cast<T>(xyz)),    y(static_cast<T>(xyz)),    z(static_cast<T>(xyz))    {}
	oHLSL3 operator-() const { return oHLSL3(-x, -y, -z); }
	oHLSL_MEMBERS(3)
};

template<typename T> struct oHLSL4
{
	T x,y,z,w;
	typedef T element_type;
	inline                        oHLSL4()                                              noexcept                                                                                                              {}
	inline                        oHLSL4(const oHLSL4& that)                            noexcept : x(that.x),                 y(that.y),                 z(that.z),                 w(that.w)                 {}
	inline                        oHLSL4(T xyzw)                                        noexcept : x(xyzw),                   y(xyzw),                   z(xyzw),                   w(xyzw)                   {}
	inline                        oHLSL4(const oHLSL2<T>& xy, T _z, T _w)               noexcept : x(xy.x),                   y(xy.y),                   z(_z),                     w(_w)                     {}
	inline                        oHLSL4(const oHLSL3<T>& xyz, T _w)                    noexcept : x(xyz.x),                  y(xyz.y),                  z(xyz.z),                  w(_w)                     {}
	inline                        oHLSL4(const oHLSL2<T>& xy, const oHLSL2<T>& zw)      noexcept : x(xy.x),                   y(xy.y),                   z(zw.x),                   w(zw.y)                   {}
	inline                        oHLSL4(T _x, const oHLSL3<T>& yzw)                    noexcept : x(_x),                     y(yzw.y),                  z(yzw.z),                  w(yzw.w)                  {}
	inline                        oHLSL4(T _x, T _y, T _z, T _w)                        noexcept : x(_x),                     y(_y),                     z(_z),                     w(_w)                     {}
	template<typename U>          oHLSL4(const oHLSL4<U>& that)                         noexcept : x(static_cast<T>(that.x)), y(static_cast<T>(that.y)), z(static_cast<T>(that.z)), w(static_cast<T>(that.w)) {}
	template<typename U> explicit oHLSL4(U xyzw)                                        noexcept : x(static_cast<T>(xyzw)),   y(static_cast<T>(xyzw)),   z(static_cast<T>(xyzw)),   w(static_cast<T>(xyzw))   {}
	template<typename U> explicit oHLSL4(const oHLSL3<U>& xyz, U _w)                    noexcept : x((T)xyz.x),               y((T)xyz.y),               z((T)xyz.z),               w((T)_w)                  {}
	template<typename U> explicit oHLSL4(const oHLSL3<U>& xy, U _z, U _w)               noexcept : x((T)xy.x),                y((T)xy.y),                z((T)_z),                  w((T)_w)                  {}
	oHLSL4 operator-() const { return oHLSL4(-x, -y, -z, -w); }
	oHLSL_MEMBERS(4)
	inline oHLSL2<T>& xy()  { return *(oHLSL2<T>*)&x; }
	inline oHLSL2<T>& zw()  { return *(oHLSL2<T>*)&z; }
	inline oHLSL3<T>& xyz() { return *(oHLSL3<T>*)&x; }
};

template<typename T> struct oHLSL4x4;
template<typename T> struct oHLSL3x3
{
	// Column-major 3x3 matrix
	oHLSL3<T> col0,col1,col2;
	typedef T element_type;
	typedef oHLSL3<T> vector_type;
	oHLSL3x3()                                                                                                                  {}
	oHLSL3x3(const oHLSL3x3& m)                                                      : col0(m.col0), col1(m.col1), col2(m.col2) {}
	oHLSL3x3(const oHLSL3<T>& _col0, const oHLSL3<T>& _col1, const oHLSL3<T>& _col2) : col0(_col0),  col1(_col1),  col2(_col2)  {}
	operator oHLSL4x4<T>() const;
	oHLSL_MAT_BRACKET_OP(oHLSL3<T>)
};

template<typename T> struct oHLSL3x4
{
	// Column-major 3x4 matrix
	oHLSL4<T> col0, col1, col2;
	typedef T element_type;
	typedef oHLSL4<T> vector_type;
	oHLSL3x4() {}
	oHLSL3x4(const oHLSL3x4& m)                                                      : col0(m.col0),      col1(m.col1),      col2(m.col2)      {}
	oHLSL3x4(const oHLSL3x3<T>& m)                                                   : col0(m.col0, 0),   col1(m.col1, 0),   col2(m.col2, 0)   {}
	oHLSL3x4(const oHLSL4<T>& _col0, const oHLSL4<T>& _col1, const oHLSL4<T>& _col2) : col0(_col0),       col1(_col1),       col2(_col2)       {}
	oHLSL3x4(const oHLSL3<T>& _col0, const oHLSL3<T>& _col1, const oHLSL3<T>& _col2) : col0(_col0, 0),    col1(_col1, 0),    col2(_col2, 0)    {}
	operator oHLSL3x3<T>() const { return oHLSL3x3<T>(col0.xyz(), col1.xyz(), col2.xyz()); }
	operator oHLSL4x4<T>() const;
	oHLSL_MAT_BRACKET_OP(oHLSL4<T>)
};

template<typename T> struct oHLSL4x4
{
	// Column-major 4x4 matrix
	oHLSL4<T> col0, col1, col2, col3;
	typedef T element_type;
	typedef oHLSL4<T> vector_type;
	oHLSL4x4() {}
	oHLSL4x4(const oHLSL4x4& m)                                                                              : col0(m.col0),    col1(m.col1),    col2(m.col2),    col3(m.col3) {}
	oHLSL4x4(const oHLSL4<T>& _col0, const oHLSL4<T>& _col1, const oHLSL4<T>& _col2, const oHLSL4<T>& _col3) : col0(_col0),     col1(_col1),     col2(_col2),     col3(_col3)  {}
	oHLSL4x4(const oHLSL3x3<T>& m, const oHLSL3<T>& tx = oHLSL3<T>(0, 0, 0))                                 : col0(m.col0, 0), col1(m.col1, 0), col2(m.col2, 0), col3(tx, 1)  {}
	operator oHLSL3x3<T>() const { return oHLSL3x3<T>(col0.xyz(), col1.xyz(), col2.xyz()); }
	oHLSL_MAT_BRACKET_OP(oHLSL4<T>)
};

template<typename T> oHLSL3x3<T>::operator oHLSL4x4<T>() const { return oHLSL4x4<T>(oHLSL4<T>(col0, 0), oHLSL4<T>(col1, 0), oHLSL4<T>(col2, 0), oHLSL4<T>(0, 0, 0, 1)); }
template<typename T> oHLSL3x4<T>::operator oHLSL4x4<T>() const { return oHLSL4x4<T>(col0, col1, col2, oHLSL4<T>(0, 0, 0, 1)); }

template<typename T> void swap_if_greater (oHLSL2<T>& a, oHLSL2<T>& b)                 { if (a.x > b.x) std::swap(a.x, b.x); if (a.y > b.y) std::swap(a.y, b.y); }
template<typename T> void swap_if_greater (oHLSL3<T>& a, oHLSL3<T>& b)                 { if (a.x > b.x) std::swap(a.x, b.x); if (a.y > b.y) std::swap(a.y, b.y); if (a.z > b.z) std::swap(a.z, b.z); }
template<typename T> void swap_if_greater (oHLSL4<T>& a, oHLSL4<T>& b)                 { if (a.x > b.x) std::swap(a.x, b.x); if (a.y > b.y) std::swap(a.y, b.y); if (a.z > b.z) std::swap(a.z, b.z); if (a.w > b.w) std::swap(a.w, b.w); }
template<typename T> void swap_if_lesser  (oHLSL2<T>& a, oHLSL2<T>& b)                 { if (a.x < b.x) std::swap(a.x, b.x); if (a.y < b.y) std::swap(a.y, b.y); }
template<typename T> void swap_if_lesser  (oHLSL3<T>& a, oHLSL3<T>& b)                 { if (a.x < b.x) std::swap(a.x, b.x); if (a.y < b.y) std::swap(a.y, b.y); if (a.z < b.z) std::swap(a.z, b.z); }
template<typename T> void swap_if_lesser  (oHLSL4<T>& a, oHLSL4<T>& b)                 { if (a.x < b.x) std::swap(a.x, b.x); if (a.y < b.y) std::swap(a.y, b.y); if (a.z < b.z) std::swap(a.z, b.z); if (a.w < b.w) std::swap(a.w, b.w); }
template<typename T> oHLSL2<T>   operator-(const oHLSL2<T>&   a)                       { return oHLSL2<T>(-a.x, -a.y); }
template<typename T> oHLSL3<T>   operator-(const oHLSL3<T>&   a)                       { return oHLSL3<T>(-a.x, -a.y, -a.z); }
template<typename T> oHLSL4<T>   operator-(const oHLSL4<T>&   a)                       { return oHLSL4<T>(-a.x, -a.y, -a.z, -a.w); }
template<typename T> oHLSL3<T>   operator*(const oHLSL3x3<T>& a, const oHLSL3<T>& b)   { return mul(a, b); }
template<typename T> oHLSL3x3<T> operator*(const oHLSL3x3<T>& a, const oHLSL3x3<T>& b) { return mul(a, b); }
template<typename T> oHLSL3<T>   operator*(const oHLSL4x4<T>& a, const oHLSL3<T>& b)   { return mul(a, b); }
template<typename T> oHLSL4<T>   operator*(const oHLSL4x4<T>& a, const oHLSL4<T>& b)   { return mul(a, b); }
template<typename T> oHLSL4x4<T> operator*(const oHLSL4x4<T>& a, const oHLSL4x4<T>& b) { return mul(a, b); }

oHLSL_OPS(2) oHLSL_OPS(3) oHLSL_OPS(4)

// _____________________________________________________________________________
// Basic types

typedef unsigned int uint;

typedef oHLSL2<float>    float2;    typedef oHLSL3<float>    float3;    typedef oHLSL4<float>    float4;     
typedef oHLSL2<double>   double2;   typedef oHLSL3<double>   double3;   typedef oHLSL4<double>   double4;
typedef oHLSL2<int>      int2;      typedef oHLSL3<int>      int3;      typedef oHLSL4<int>      int4;
typedef oHLSL2<uint>     uint2;     typedef oHLSL3<uint>     uint3;     typedef oHLSL4<uint>     uint4;
typedef oHLSL2<bool>     bool2;     typedef oHLSL3<bool>     bool3;     typedef oHLSL4<bool>     bool4;
typedef oHLSL3x3<float>  float3x3;  typedef oHLSL4x4<float>  float4x4;  typedef oHLSL3x4<float>  float3x4;
typedef oHLSL3x3<double> double3x3; typedef oHLSL4x4<double> double4x4; typedef oHLSL3x4<double> double3x4;

template<typename T> struct is_hlsl
{
	static const bool value = 
		std::is_floating_point<T>::value ||
		std::is_same<bool,     std::remove_cv<T>::type>::value ||
		std::is_same<bool2,    std::remove_cv<T>::type>::value ||
		std::is_same<bool3,    std::remove_cv<T>::type>::value ||
		std::is_same<bool4,    std::remove_cv<T>::type>::value ||
		std::is_same<int,      std::remove_cv<T>::type>::value ||
		std::is_same<uint,     std::remove_cv<T>::type>::value ||
		std::is_same<int2,     std::remove_cv<T>::type>::value ||
		std::is_same<int3,     std::remove_cv<T>::type>::value ||
		std::is_same<int4,     std::remove_cv<T>::type>::value ||
		std::is_same<uint2,    std::remove_cv<T>::type>::value ||
		std::is_same<uint3,    std::remove_cv<T>::type>::value ||
		std::is_same<uint4,    std::remove_cv<T>::type>::value ||
		std::is_same<float2,   std::remove_cv<T>::type>::value ||
		std::is_same<float3,   std::remove_cv<T>::type>::value ||
		std::is_same<float4,   std::remove_cv<T>::type>::value ||
		std::is_same<double2,  std::remove_cv<T>::type>::value ||
		std::is_same<double3,  std::remove_cv<T>::type>::value ||
		std::is_same<double4,  std::remove_cv<T>::type>::value ||
		std::is_same<float3x3, std::remove_cv<T>::type>::value ||
		std::is_same<float4x4, std::remove_cv<T>::type>::value ||
		std::is_same<double3x3,std::remove_cv<T>::type>::value ||
		std::is_same<double4x4,std::remove_cv<T>::type>::value;
};

// _____________________________________________________________________________
// Atomics

inline void InterlockedCompareExchange(int&           dest, int           cmp, int           value, int&           orig) { orig = (int)_InterlockedCompareExchange  ((volatile long*)&dest,        (long)value,     (long)cmp);                             }
inline void InterlockedCompareExchange(long&          dest, long          cmp, long          value, long&          orig) { orig =      _InterlockedCompareExchange  (                &dest,              value,           cmp);                             }
inline void InterlockedCompareExchange(long long&     dest, long long     cmp, long long     value, long long&     orig) { orig =      _InterlockedCompareExchange64(                &dest,              value,           cmp);                             }
inline void InterlockedCompareExchange(unsigned int&  dest, unsigned int  cmp, unsigned int  value, unsigned int&  orig) { long v =    _InterlockedCompareExchange  ((volatile long*)&dest,      *(long*)&value, *(long*)&cmp); orig = *(unsigned int*) &v; }
inline void InterlockedCompareExchange(unsigned long& dest, unsigned long cmp, unsigned long value, unsigned long& orig) { long v =    _InterlockedCompareExchange  ((volatile long*)&dest,      *(long*)&value, *(long*)&cmp); orig = *(unsigned long*)&v; }

inline void InterlockedCompareStore(int&              dest, int           cmp, int           value)                      {             _InterlockedCompareExchange  ((volatile long*)&dest,         (long)value,    (long)cmp);                             }
inline void InterlockedCompareStore(long&             dest, long          cmp, long          value)                      {             _InterlockedCompareExchange  (                &dest,               value,          cmp);                             }
inline void InterlockedCompareStore(unsigned int&     dest, unsigned int  cmp, unsigned int  value)                      { long v =    _InterlockedCompareExchange  ((volatile long*)&dest,      *(long*)&value, *(long*)&cmp);                             }
inline void InterlockedCompareStore(unsigned long&    dest, unsigned long cmp, unsigned long value)                      { long v =    _InterlockedCompareExchange  ((volatile long*)&dest,      *(long*)&value, *(long*)&cmp);                             }

inline void InterlockedExchange(int&                  dest,                    int                value, int&           orig) { orig = (int)_InterlockedExchange    ((volatile long*)&dest,     *(long*)&value);                                            }
inline void InterlockedExchange(long&                 dest,                    long               value, long&          orig) { orig =      _InterlockedExchange    (                &dest,              value);                                            }
inline void InterlockedExchange(unsigned int&         dest,                    unsigned int       value, unsigned int&  orig) { long v =    _InterlockedExchange    ((volatile long*)&dest,     *(long*)&value); orig = *(unsigned int*) &v;                }
inline void InterlockedExchange(unsigned long&        dest,                    unsigned long      value, unsigned long& orig) { long v =    _InterlockedExchange    ((volatile long*)&dest,     *(long*)&value); orig = *(unsigned long*)&v;                }
inline void InterlockedExchange(int&                  dest,                    int                value)                      {             _InterlockedExchange    ((volatile long*)&dest,     *(long*)&value);                                            }
inline void InterlockedExchange(long&                 dest,                    long               value)                      {             _InterlockedExchange    (                &dest,              value);                                            }
inline void InterlockedExchange(unsigned int&         dest,                    unsigned int       value)                      {             _InterlockedExchange    ((volatile long*)&dest,     *(long*)&value);                                            }
inline void InterlockedExchange(unsigned long&        dest,                    unsigned long      value)                      {             _InterlockedExchange    ((volatile long*)&dest,     *(long*)&value);                                            }

inline void InterlockedAdd(int&                       dest,                    int                value, long&          orig) { orig = (int)_InterlockedExchangeAdd((volatile long*)&dest,         (long)value);                                            }
inline void InterlockedAdd(long&                      dest,                    long               value, long&          orig) { orig =      _InterlockedExchangeAdd(                &dest,               value);                                            }
inline void InterlockedAdd(unsigned int&              dest,                    unsigned int       value, unsigned int&  orig) { long v =    _InterlockedExchangeAdd((volatile long*)&dest,         (long)value); orig = *(unsigned int*) &v;                }
inline void InterlockedAdd(unsigned long&             dest,                    unsigned long      value, unsigned long& orig) { long v =    _InterlockedExchangeAdd((volatile long*)&dest,      *(long*)&value); orig = *(unsigned long*)&v;                }
inline void InterlockedAdd(int&                       dest,                    int                value)                      {             _InterlockedExchangeAdd((volatile long*)&dest,         (long)value);                                            }
inline void InterlockedAdd(long&                      dest,                    long               value)                      {             _InterlockedExchangeAdd(                &dest,               value);                                            }
inline void InterlockedAdd(unsigned int&              dest,                    unsigned int       value)                      {             _InterlockedExchangeAdd((volatile long*)&dest,         (long)value);                                            }
inline void InterlockedAdd(unsigned long&             dest,                    unsigned long      value)                      {             _InterlockedExchangeAdd((volatile long*)&dest,      *(long*)&value);                                            }

inline void InterlockedAnd(int&                       dest,                    int                value)                      {             _InterlockedAnd        ((volatile long*)&dest,         (long)value);                                            }
inline void InterlockedAnd(long&                      dest,                    long               value)                      {             _InterlockedAnd        (                &dest,               value);                                            }
inline void InterlockedAnd(unsigned int&              dest,                    unsigned int       value)                      {             _InterlockedAnd        ((volatile long*)&dest,      *(long*)&value);                                            }
inline void InterlockedAnd(unsigned long&             dest,                    unsigned long      value)                      {             _InterlockedAnd        ((volatile long*)&dest,      *(long*)&value);                                            }
inline void InterlockedAnd(int&                       dest,                    int                value, int&           orig) { orig = (int)_InterlockedAnd        ((volatile long*)&dest,         (long)value);                                            }
inline void InterlockedAnd(long&                      dest,                    long               value, long&          orig) { orig =      _InterlockedAnd        (                &dest,               value);                                            }
inline void InterlockedAnd(unsigned int&              dest,                    unsigned int       value, unsigned int&  orig) { long v =    _InterlockedAnd        ((volatile long*)&dest,       *(long*)&value); orig = *(unsigned int*) &v;               }
inline void InterlockedAnd(unsigned long&             dest,                    unsigned long      value, unsigned long& orig) { long v =    _InterlockedAnd        ((volatile long*)&dest,       *(long*)&value); orig = *(unsigned long*)&v;               }

inline void InterlockedOr(int&                        dest,                    int                value)                      {             _InterlockedOr(         (volatile long*)&dest,         (long)value);                                            }
inline void InterlockedOr(long&                       dest,                    long               value)                      {             _InterlockedOr(                         &dest,               value);                                            }
inline void InterlockedOr(unsigned int&               dest,                    unsigned int       value)                      {             _InterlockedOr(         (volatile long*)&dest,      *(long*)&value);                                            }
inline void InterlockedOr(unsigned long&              dest,                    unsigned long      value)                      {             _InterlockedOr(         (volatile long*)&dest,      *(long*)&value);                                            }
inline void InterlockedOr(int&                        dest,                    int                value, int&           orig) { orig = (int)_InterlockedOr(         (volatile long*)&dest,         (long)value);                                            }
inline void InterlockedOr(long&                       dest,                    long               value, long&          orig) { orig =      _InterlockedOr(                         &dest,               value);                                            }
inline void InterlockedOr(unsigned int&               dest,                    unsigned int       value, unsigned int&  orig) { long v =    _InterlockedOr(         (volatile long*)&dest,      *(long*)&value); orig = *(unsigned int*) &v;                }
inline void InterlockedOr(unsigned long&              dest,                    unsigned long      value, unsigned long& orig) { long v =    _InterlockedOr(         (volatile long*)&dest,      *(long*)&value); orig = *(unsigned long*)&v;                }

inline void InterlockedXor(int&                       dest,                    int                value)                      {             _InterlockedXor  ((volatile long*)      &dest,         (long)value);                                            }
inline void InterlockedXor(long&                      dest,                    long               value)                      {             _InterlockedXor  (                      &dest,               value);                                            }
inline void InterlockedXor(unsigned int&              dest,                    unsigned int       value)                      {             _InterlockedXor  ((volatile long*)      &dest,      *(long*)&value);                                            }
inline void InterlockedXor(unsigned long&             dest,                    unsigned long      value)                      {             _InterlockedXor  ((volatile long*)      &dest,      *(long*)&value);                                            }
inline void InterlockedXor(unsigned long long&        dest,                    unsigned long long value)                      {             _InterlockedXor64((volatile long long*) &dest, *(long long*)&value);                                            }
inline void InterlockedXor(int&                       dest,                    int                value, int&           orig) { orig = (int)_InterlockedXor  ((volatile long*)      &dest,         (long)value);                                            }
inline void InterlockedXor(long&                      dest,                    long               value, long&          orig) { orig =      _InterlockedXor  (                      &dest,               value);                                            }
inline void InterlockedXor(unsigned int&              dest,                    unsigned int       value, unsigned int&  orig) { long v =    _InterlockedXor  ((volatile long*)      &dest,      *(long*)&value); orig = *(unsigned int*) &v;                }
inline void InterlockedXor(unsigned long&             dest,                    unsigned long      value, unsigned long& orig) { long v =    _InterlockedXor  ((volatile long*)      &dest,      *(long*)&value); orig = *(unsigned long*)&v;                }

// _____________________________________________________________________________
// Bits

inline uint countbits(uint x)
{
#if 1
  return __popcnt(x);
#else
	// http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	x = x - ((x >> 1) & 0x55555555);
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	return ((x + (x >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
}

inline uint reversebits(uint x)
{
	// http://graphics.stanford.edu/~seander/bithacks.html#ReverseParallel
	x = ((x >>  1) & 0x55555555) | ((x & 0x55555555) <<   1); // swap odd and even bits
	x = ((x >>  2) & 0x33333333) | ((x & 0x33333333) <<   2); // swap consecutive pairs
	x = ((x >>  4) & 0x0F0F0F0F) | ((x & 0x0F0F0F0F) <<   4); // swap nibbles ... 
	x = ((x >>  8) & 0x00FF00FF) | ((x & 0x00FF00FF) <<   8); // swap bytes
	x = ((x >> 16)             ) | ((x             )  << 16); // swap 2-byte long pairs
	return x;
}

inline int reversebits(int x) { uint r = reversebits(*(uint*)&x); return *(int*)&r; }

// firstbit[high|low|set] return -1 if x is 0 or the index of the first bit set.
inline int firstbithigh(uint x) { unsigned long index; return _BitScanReverse(&index, x) == 0 ? -1 : index; }
inline int firstbitlow(uint x)  { unsigned long index; return _BitScanForward(&index, x) == 0 ? -1 : index; }
inline int firstbitset(uint x)  { return x ? (x & (~x + 1)) : -1; }

// _____________________________________________________________________________
// Float Introspection

template<typename T> T sign(const T& x) { return x < 0 ? T(-1) : (x == 0 ? T(0) : T(1)); }
template<typename T> oHLSL3<T> sign(const oHLSL3<T>& v){ return oHLSL3<T>(sign(v.x), sign(v.y), sign(v.z));}
inline bool isfinite(const float& a)
{
	#ifdef _M_X64
		return !!_finitef(a);
	#else
		return isfinite(a);
	#endif
}

inline bool isnan(const float& a)
{
	#ifdef _M_X64
		return !!_isnanf(a);
	#else
		return !!_isnan((double)a);
	#endif
}

inline bool isfinite(const double& a) { return !!_finite(a); }
template<typename T> bool isinf(const T& a) { return !isfinite(a); }
inline bool isinf(const double& a) { return !isfinite(a); }
inline bool isnan(const double& a) { return !!_isnan(a); }
template<typename T> bool isfinite(const oHLSL2<T>& a) { return isfinite(a.x) && isfinite(a.y); }
template<typename T> bool isfinite(const oHLSL3<T>& a) { return isfinite(a.x) && isfinite(a.y) && isfinite(a.z); }
template<typename T> bool isfinite(const oHLSL4<T>& a) { return isfinite(a.x) && isfinite(a.y) && isfinite(a.z) && isfinite(a.w); }
template<typename T> bool isinf(const oHLSL2<T>& a) { return isinf(a.x) || isinf(a.y); }
template<typename T> bool isinf(const oHLSL3<T>& a) { return isinf(a.x) || isinf(a.y) || isinf(a.z); }
template<typename T> bool isinf(const oHLSL4<T>& a) { return isinf(a.x) || isinf(a.y) || isinf(a.z) || isinf(a.w); }
template<typename T> bool isnan(const oHLSL2<T>& a) { return isnan(a.x) || isnan(a.y); }
template<typename T> bool isnan(const oHLSL3<T>& a) { return isnan(a.x) || isnan(a.y) || isnan(a.z); }
template<typename T> bool isnan(const oHLSL4<T>& a) { return isnan(a.x) || isnan(a.y) || isnan(a.z) || isnan(a.w); }
template<typename T> bool isdenorm(const oHLSL2<T>& a) { return isdenorm(a.x) || isdenorm(a.y); }
template<typename T> bool isdenorm(const oHLSL3<T>& a) { return isdenorm(a.x) || isdenorm(a.y) || isdenorm(a.z); }
template<typename T> bool isdenorm(const oHLSL4<T>& a) { return isdenorm(a.x) || isdenorm(a.y) || isdenorm(a.z) || isdenorm(a.w); }

// _____________________________________________________________________________
// Type conversions

inline int asint(float f)
{
	#if oHLSL_USE_FAST_ASINT
		// Blow, Jonathon. "Unified Rendering Level-of-Detail, Part 2." 
		// Game Developer Magazine, April 2003.
		// http://www.gdmag.com/resources/code.htm
		/*
			This file contains the necessary code to do a fast float-to-int
			conversion, where the input is an IEEE-754 32-bit floating-point
			number.  Just call FastInt(f).  The idea here is that C and C++
			have to do some extremely slow things in order to ensure proper
			rounding semantics; if you don't care about your integer result
			precisely conforming to the language spec, then you can use this.
			FastInt(f) is many many many times faster than (int)(f) in almost
			all cases.
		*/
		const unsigned int DOPED_MAGIC_NUMBER_32 = 0x4b3fffff;
		f += (float &)DOPED_MAGIC_NUMBER_32;
		int result = (*(unsigned int*)&f) - DOPED_MAGIC_NUMBER_32;
		return result;
	#else
		return static_cast<int>(f);
	#endif
}

struct oHLSLSW64
{	union
	{
		double asdouble;
		float  asfloat[2];
		int    asint[2];
		uint   asuint[2];
};};

inline               double    asdouble(uint lowbits, uint highbits)                 { oHLSLSW64 s; s.asuint[0] = lowbits; s.asuint[1] = highbits; return s.asdouble; }
inline               double2   asdouble(const uint2& lowbits, const uint2& highbits) { oHLSLSW64 s[2]; s[0].asuint[0] = lowbits.x; s[0].asuint[1] = highbits.x; s[1].asuint[0] = lowbits.y; s[1].asuint[1] = highbits.y; return double2(s[0].asdouble, s[1].asdouble); }
template<typename T> double    asdouble(const T& x)                                  { return *(double*)&x; }
template<typename T> double2   asdouble(const oHLSL2<T>& x)                          { return double2(asdouble(x.x), asdouble(x.y)); }
template<typename T> double3   asdouble(const oHLSL3<T>& x)                          { return double3(asdouble(x.x), asdouble(x.y), asdouble(x.z)); }
template<typename T> double4   asdouble(const oHLSL4<T>& x)                          { return double4(asdouble(x.x), asdouble(x.y), asdouble(x.z), asdouble(x.w)); }
template<typename T> double4x4 asdouble(const oHLSL4x4<T>& x)                        { return double4x4(asdouble(x.col0), asdouble(x.col1), asdouble(x.col2), asdouble(x.col3)); }
inline               float2    asfloat(const double& x)                              { oHLSLSW64 s; s.asdouble = x; return float2(s.asfloat[0], s.asfloat[1]); }
inline               float4    asfloat(const double2& x)                             { oHLSLSW64 s[2]; s[0].asdouble = x.x; s[1].asdouble = x.y; return float4(s[0].asfloat[0], s[0].asfloat[1], s[1].asfloat[0], s[1].asfloat[1]); }
template<typename T> float     asfloat(const T& x)                                   { return *(float*)&x; }
template<typename T> float2    asfloat(const oHLSL2<T>& x)                           { return float2(asfloat(x.x), asfloat(x.y)); }
template<typename T> float3    asfloat(const oHLSL3<T>& x)                           { return float3(asfloat(x.x), asfloat(x.y), asfloat(x.z)); }
template<typename T> float4    asfloat(const oHLSL4<T>& x)                           { return float4(asfloat(x.x), asfloat(x.y), asfloat(x.z), asfloat(x.w)); }
template<typename T> float4x4  asfloat(const oHLSL4x4<T>& x)                         { return float4x4(asfloat(x.col0), asfloat(x.col1), asfloat(x.col2), asfloat(x.col3)); }
inline               int2      asint(double x)                                       { oHLSLSW64 s; s.asdouble = x; return int2(s.asint[0], s.asint[1]); }
inline               int4      asint(double2 x)                                      { oHLSLSW64 s[2]; s[0].asdouble = x.x; s[1].asdouble = x.y; return int4(s[0].asint[0], s[0].asint[1], s[1].asint[0], s[1].asint[1]); }
template<typename T> int       asint(const T& x)                                     { return *(int*)&x; }
template<typename T> int2      asint(const oHLSL2<T>& x)                             { return int2(asint(x.x), asint(x.y)); }
template<typename T> int3      asint(const oHLSL3<T>& x)                             { return int3(asint(x.x), asint(x.y), asint(x.z)); }
template<typename T> int4      asint(const oHLSL4<T>& x)                             { return int4(asint(x.x), asint(x.y), asint(x.z), asint(x.w)); }
inline               void      asuint(double x, uint& a, uint& b)                    { oHLSLSW64 s; s.asdouble = x; a = s.asuint[0]; b = s.asuint[1]; }
template<typename T> uint      asuint(const T& x)                                    { return *(uint*)&x; }
template<typename T> uint2     asuint(const oHLSL2<T>& x)                            { return uint2(asuint(x.x), asuint(x.y)); }
template<typename T> uint3     asuint(const oHLSL3<T>& x)                            { return uint3(asuint(x.x), asuint(x.y), asuint(x.z)); }
template<typename T> uint4     asuint(const oHLSL4<T>& x)                            { return uint4(asuint(x.x), asuint(x.y), asuint(x.z), asuint(x.w)); }

#ifdef _HALF_H_
inline               float  f16tof32(uint x)             { half h; h.setBits(static_cast<unsigned short>(x)); return static_cast<float>(h); }
inline               uint   f32tof16(float x)            { return half(x).bits(); }
template<typename T> float2 f16tof32(const uint2& value) { return float2(f16tof32(value.x), f16tof32(value.y)); }
template<typename T> float3 f16tof32(const uint3& value) { return float2(f16tof32(value.x), f16tof32(value.y), f16tof32(value.z)); }
template<typename T> float4 f16tof32(const uint4& value) { return float2(f16tof32(value.x), f16tof32(value.y), f16tof32(value.z), f16tof32(value.w)); }
template<typename T> uint2  f32tof16(const uint2& value) { return uint2(f32tof16(value.x), f32tof16(value.y)); }
template<typename T> uint3  f32tof16(const uint3& value) { return uint3(f32tof16(value.x), f32tof16(value.y), f32tof16(value.z)); }
template<typename T> uint4  f32tof16(const uint4& value) { return uint4(f32tof16(value.x), f32tof16(value.y), f32tof16(value.z), f32tof16(value.w)); }
#endif

// _____________________________________________________________________________
// Selection

inline bool all(const bool& a)         { return a;                        }
inline bool all(const oHLSL2<bool>& a) { return a.x && a.y;               }
inline bool all(const oHLSL3<bool>& a) { return a.x && a.y && a.z;        }
inline bool all(const oHLSL4<bool>& a) { return a.x && a.y && a.z && a.w; }

inline bool any(const bool& a)         { return a;                        }
inline bool any(const oHLSL2<bool>& a) { return a.x || a.y;               }
inline bool any(const oHLSL3<bool>& a) { return a.x || a.y || a.z;        }
inline bool any(const oHLSL4<bool>& a) { return a.x || a.y || a.z || a.w; }

// _____________________________________________________________________________
// Trigonometry

oHLSL_FN1S(cos); oHLSL_FN1S(acos); oHLSL_FN1S(cosh);
oHLSL_FN1S(sin); oHLSL_FN1S(asin); oHLSL_FN1S(sinh);
oHLSL_FN1S(tan); oHLSL_FN1S(atan); oHLSL_FN1S(tanh); oHLSL_FN2S(atan2, atan2);
template<typename T> void sincos(const T& angle_radians, T& out_sin, T& out_cos) { out_sin = sin(angle_radians); out_cos = cos(angle_radians); }

// _____________________________________________________________________________
// Geometry

oHLSL_FN1S(radians); oHLSL_FN1S(degrees);
template<typename T> oHLSL3<T>       cross(const oHLSL3<T>& a, const oHLSL3<T>& b)               { return oHLSL3<T>(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
template<typename T> T               dot(const oHLSL2<T>& a, const oHLSL2<T>& b)                 { return a.x*b.x + a.y*b.y; }
template<typename T> T               dot(const oHLSL3<T>& a, const oHLSL3<T>& b)                 { return a.x*b.x + a.y*b.y + a.z*b.z; }
template<typename T> T               dot(const oHLSL4<T>& a, const oHLSL4<T>& b)                 { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
template<typename T> T               length(const oHLSL2<T>& a)                                  { return sqrt(dot(a, a)); }
template<typename T> T               length(const oHLSL3<T>& a)                                  { return sqrt(dot(a, a)); }
template<typename T> T               length(const oHLSL4<T>& a)                                  { return sqrt(dot(a, a)); }
template<typename T> const T         lerp(const T& a, const T& b, const T& s)                    { return a + s * (b-a); }
template<typename T> const oHLSL2<T> lerp(const oHLSL2<T>& a, const oHLSL2<T>& b, const T& s)    { return a + s * (b-a); }
template<typename T> const oHLSL3<T> lerp(const oHLSL3<T>& a, const oHLSL3<T>& b, const T& s)    { return a + s * (b-a); }
template<typename T> const oHLSL4<T> lerp(const oHLSL4<T>& a, const oHLSL4<T>& b, const T& s)    { return a + s * (b-a); }
template<typename T> oHLSL4<T>       lit(const T& n_dot_l, const T& n_dot_h, const T& m)         { oHLSL4<T>(T(1), (n_dot_l < 0) ? 0 : n_dot_l, (n_dot_l < 0) || (n_dot_h < 0) ? 0 : (n_dot_h * m), T(1)); }
template<typename T> T               faceforward(const T& n, const T& i, const T& ng)            { return -n * sign(dot(i, ng)); }
template<typename T> T               normalize(const T& x)                                       { return x / length(x); }
template<typename T> T               radians(T degrees)                                          { return degrees * T(3.14159265358979323846) / T(180.0); }
template<typename T> T               degrees(T radians)                                          { return radians * T(180.0) / T(3.14159265358979323846); }
template<typename T> T               distance(const oHLSL2<T>& a, const oHLSL2<T>& b)            { return length(a-b); }
template<typename T> T               distance(const oHLSL3<T>& a, const oHLSL3<T>& b)            { return length(a-b); }
template<typename T> T               reflect(const T& i, const T& n)                             { return i - T(2.0) * n * dot(i,n); }
template<typename T> T               refract(const oHLSL3<T>& i, const oHLSL3<T>& n, const T& r) { T c1 = dot(i,n); T c2 = T(1) - r*r * (T(1) - c1*c1); return (c2 < T(0)) ? oHLSL3<T>(0) : r*i + (sqrt(c2) - r*c1) * n; } // http://www.physicsforums.com/archive/index.php/t-187091.html

// _____________________________________________________________________________
// Algebra

oHLSL_FN1S(abs); oHLSL_FN1S(ceil); oHLSL_FN1S(floor); oHLSL_FN1S(frac); oHLSL_FN1S(round);
#if (_MSC_VER < 1600)
inline long long abs(const long long& x) { return _abs64(x); }
#endif
oHLSL_FN1S(log); oHLSL_FN1S(log2); oHLSL_FN1S(log10);
oHLSL_FN1S(exp); oHLSL_FN1S(exp2);
oHLSL_FN1S(pow); oHLSL_FN1S(sqrt);
oHLSL_FN2S(ldexp, ldexp); // return x * 2^(exp) float
oHLSL_FN2S(frexp, frexp);
oHLSL_FN2S(fmod, fmod);
oHLSL_FN1S(rcp);

template<typename T> T         round(const T& a)                                 { return floor(a + T(0.5)); }
template<typename T> T         frac(const T& a)                                  { return a - floor(a); }
template<typename T> T         trunc(const T& x)                                 { return floor(x); }
template<typename T> T         frexp(const T& x, T& exp)                         { int e; T ret = ::frexp(x, &e); exp = static_cast<T>(e); return ret; }
inline               float     modf(const float& x, float& ip)          noexcept { return modff(x, &ip); }
inline               double    modf(const double& x, double& ip)        noexcept { return modf(x, &ip); }
template<typename T> oHLSL2<T> modf(const oHLSL2<T>& x, oHLSL2<T>& ip)  noexcept { return oHLSL2<T>(modf(x.x, ip.x), modf(x.y, ip.y)); }
template<typename T> oHLSL3<T> modf(const oHLSL3<T>& x, oHLSL3<T>& ip)  noexcept { return oHLSL3<T>(modf(x.x, ip.x), modf(x.y, ip.y), modf(x.z, ip.z)); }
template<typename T> oHLSL4<T> modf(const oHLSL4<T>& x, oHLSL4<T>& ip)  noexcept { return oHLSL4<T>(modf(x.x, ip.x), modf(x.y, ip.y), modf(x.z, ip.z), modf(x.w, ip.w)); }
template<typename T> T         rsqrt(T x)                                        { return T(1) / sqrt(x); }
template<typename T> T         rcp(const T& value)                               { return T(1) / value; }
#if oHAS_EXP2 == 0
inline               float     exp2(float a)                                     { return powf(2.0f, a); }
inline               double    exp2(double a)                                    { return pow(2.0, a); }
#endif
#if oHAS_LOG2 == 0
inline               double    log2(double a)                                    { static const double sCONV = 1.0/log(2.0); return log(a) * sCONV; }
inline               float     log2(float val)
{
#if oHLSL_USE_FAST_LOG2
	// Blaxill http://www.devmaster.net/forums/showthread.php?t=12765
	int * const    exp_ptr = reinterpret_cast <int *>(&val);
	int            x = *exp_ptr;
	const int      log_2 = ((x >> 23) & 255) - 128;
	x &= ~(255 << 23);
	x += 127 << 23;
	*exp_ptr = x;

	val = ((-1.0f/3) * val + 2) * val - 2.0f/3; //(1)

	return (val + log_2);
#else
	static const double sCONV = 1.0/log(2.0);
	return static_cast<float>(log(val) * sCONV);
#endif
}

#endif

template<> inline float rcp(const float& x)
{
	#if oHLSL_USE_FAST_RCP
		// Simon Hughes
		// http://www.codeproject.com/cpp/floatutils.asp?df=100&forumid=208&exp=0&select=950856#xx950856xx
		// This is about 2.12 times faster than using 1.0f / n 
		int _i = 2 * 0x3F800000 - *(int*)&x;
		float r = *(float *)&_i;
		return r * (2.0f - x * r);
	#else
		return 1.0f / x;
	#endif
}

template<> inline float rsqrt(float x)
{
	#if oHLSL_USE_FAST_RSQRT
		http://www.beyond3d.com/content/articles/8/
		float xhalf = 0.5f*x;
		int i = *(int*)&x;
		i = 0x5f3759df - (i>>1);
		x = *(float*)&i;
		x = x*(1.5f - xhalf*x*x);
		return x;
	#else
		return 1.0f / (float)sqrt(x);
	#endif
}

// _____________________________________________________________________________
// Range/clamp functions

// this error 'min' or 'max' : no overloaded function takes 2 arguments
// implies that the paramters aren't the same type and may require explicit
// casting.
oHLSL_FN2S(max, __max); oHLSL_FN2S(min, __min); oHLSL_FN2S(step, step);

template<typename T> T                         max(const T& x, const T& y)                                                        { return (x > y) ? x : y; }
template<typename T> T                         min(const T& x, const T& y)                                                        { return (x < y) ? x : y; }
template<typename T> T                         step(const T& y, const T& x)                                                       { return (x >= y) ? T(1) : T(0); } 
template<typename T> oHLSL2<T>                 smoothstep(const oHLSL2<T>& minimum, const oHLSL2<T>& maximum, const oHLSL2<T>& x) { return oHLSL2<T>(smoothstep(minimum.x, maximum.x, x.x), smoothstep(minimum.y, maximum.y, x.y)); }
template<typename T> oHLSL3<T>                 smoothstep(const oHLSL3<T>& minimum, const oHLSL3<T>& maximum, const oHLSL3<T>& x) { return oHLSL3<T>(smoothstep(minimum.x, maximum.x, x.x), smoothstep(minimum.y, maximum.y, x.y), smoothstep(minimum.z, maximum.z, x.z)); }
template<typename T> oHLSL4<T>                 smoothstep(const oHLSL4<T>& minimum, const oHLSL4<T>& maximum, const oHLSL4<T>& x) { return oHLSL4<T>(smoothstep(minimum.x, maximum.x, x.x), smoothstep(minimum.y, maximum.y, x.y), smoothstep(minimum.z, maximum.z, x.z), smoothstep(minimum.w, maximum.w, x.w)); }
template<typename T> T                         smoothstep(const T& minimum, const T& maximum, const T& x)                         { T t = saturate((x - minimum)/(maximum-minimum)); return t*t*(T(3) - T(2)*t); } // http://http.developer.nvidia.com/Cg/smoothstep.html
template<typename T> T                         clamp(const T& x, const T& minimum, const T& maximum)                              { return ::max(::min(x, maximum), minimum); }
template<typename T, typename vectorT> vectorT clamp(const vectorT& x, const T& minimum, const T& maximum)                        { return clamp<vectorT>(x, vectorT(minimum), vectorT(maximum)); }
template<typename T> T                         saturate(const T& x)                                                               { return clamp<T>(x, T(0.0), T(1.0)); }

// _____________________________________________________________________________
// Operators

// mul(a,b) means a * b, meaning if you want to scale then translate, it would be result = scale * translate
template<typename T> oHLSL3x3<T> mul(const oHLSL3x3<T>& a, const oHLSL3x3<T>& b) { return oHLSL3x3<T>(b*a[0], b*a[1], b*a[2]); }
template<typename T> oHLSL4x4<T> mul(const oHLSL4x4<T>& a, const oHLSL4x4<T>& b) { return oHLSL4x4<T>(b*a[0], b*a[1], b*a[2], b*a[3]); }
template<typename T> oHLSL3<T>   mul(const oHLSL3x3<T>& a, const oHLSL3<T>&   b) { return oHLSL3<T>(a[0].x*b.x + a[1].x*b.y + a[2].x*b.z, a[0].y*b.x + a[1].y*b.y + a[2].y*b.z, a[0].z*b.x + a[1].z*b.y + a[2].z*b.z); }
template<typename T> oHLSL3<T>   mul(const oHLSL4x4<T>& a, const oHLSL3<T>&   b) { return mul(a, oHLSL4<T>(b,T(1))).xyz(); }
template<typename T> oHLSL4<T>   mul(const oHLSL4x4<T>& a, const oHLSL4<T>&   b) { return oHLSL4<T>(((((a[0].x*b.x) + (a[1].x*b.y)) + (a[2].x*b.z)) + (a[3].x*b.w)), ((((a[0].y*b.x) + (a[1].y*b.y)) + (a[2].y*b.z)) + (a[3].y*b.w)), ((((a[0].z*b.x) + (a[1].z*b.y)) + (a[2].z*b.z)) + (a[3].z*b.w)), ((((a[0].w*b.x) + (a[1].w*b.y)) + (a[2].w*b.z)) + (a[3].w*b.w))); }
template<typename T> T           mad(const T& m, const T& a, const T& b)         { return m*a + b; }

// _____________________________________________________________________________
// HLSL Matrix operations

template<typename T> T determinant(const oHLSL3x3<T>& m) { return dot(m[2], cross(m[0], m[1])); }
template<typename T> T determinant(const oHLSL4x4<T>& m)
{
	// Erwin Coumans (attributed)
	// http://continuousphysics.com/Bullet/
	T dx, dy, dz, dw, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5;
	T mA = m[0].x, mB = m[0].y, mC = m[0].z, mD = m[0].w;
	T mE = m[1].x, mF = m[1].y, mG = m[1].z, mH = m[1].w;
	T mI = m[2].x, mJ = m[2].y, mK = m[2].z, mL = m[2].w;
	T mM = m[3].x, mN = m[3].y, mO = m[3].z, mP = m[3].w;
	tmp0 = mK*mD - mC*mL; tmp1 = mO*mH - mG*mP; tmp2 = mB*mK - mJ*mC;               tmp3 = mF*mO - mN*mG;               tmp4 = mJ*mD - mB*mL;               tmp5 = mN*mH - mF*mP;
	dx   = mJ*tmp1 - mL*tmp3 - mK*tmp5;         dy   = mN*tmp0 - mP*tmp2 - mO*tmp4; dz   = mD*tmp3 + mC*tmp5 - mB*tmp1; dw   = mH*tmp2 + mG*tmp4 - mF*tmp0;
	return mA*dx + mE*dy + mI*dz + mM*dw;
}

template<typename T> oHLSL3x3<T> transpose(const oHLSL3x3<T>& m) { return oHLSL3x3<T>(oHLSL3<T>(m[0].x, m[1].x, m[2].x        ), oHLSL3<T>(m[0].y, m[1].y, m[2].y        ), oHLSL3<T>(m[0].z, m[1].z, m[2].z        ));                                            }
template<typename T> oHLSL4x4<T> transpose(const oHLSL4x4<T>& m) { return oHLSL4x4<T>(oHLSL4<T>(m[0].x, m[1].x, m[2].x, m[3].x), oHLSL4<T>(m[0].y, m[1].y, m[2].y, m[3].y), oHLSL4<T>(m[0].z, m[1].z, m[2].z, m[3].z), oHLSL4<T>(m[0].w, m[1].w, m[2].w, m[3].w)); }

// _____________________________________________________________________________
// Structs

#define oHLSL_OP_ARRAY             T& operator[](uint i)       { assert(i < capacity && "out of range"); return buffer[i]; }
#define oHLSL_CONST_OP_ARRAY const T& operator[](uint i) const { assert(i < capacity && "out of range"); return buffer[i]; }

template<typename T> class HLSLBufferBase
{
public:
	~HLSLBufferBase() { Deallocate(); }

	void GetDimensions(uint& num_structs, uint& stride) const { num_structs = capacity; stride = sizeof(T); }

	oHLSL_CONST_OP_ARRAY

  // === non-specification helpers ===

	void Reallocate(uint capacity) { if (capacity > this->capacity) { Deallocate(); buffer = new T[capacity]; this->capacity = capacity; } }
	void Deallocate() { if (buffer) delete [] buffer; }

	void CopyFrom(const void* src, uint src_size)
	{
		assert(src_size <= (capacity * sizeof(T)) && "invalidly sized buffer");
		memcpy(buffer, src, src_size);
	}

protected:
	HLSLBufferBase() : buffer(nullptr), capacity(0) {}
	HLSLBufferBase(uint capacity) : capacity(capacity) { Reallocate(capacity); }

	T* buffer;
	uint capacity;

private:
	HLSLBufferBase(const HLSLBufferBase&);
	const HLSLBufferBase& operator=(const HLSLBufferBase&);
};

template<typename T> class HLSLCounterBufferBase : public HLSLBufferBase<T>
{
	HLSLCounterBufferBase(const HLSLCounterBufferBase&);
	const HLSLCounterBufferBase& operator=(const HLSLCounterBufferBase&);

public:
	void SetCounter(uint counter) { this->counter = counter; }
	uint GetCounter() const { return counter; }

	oHLSL_OP_ARRAY
	oHLSL_CONST_OP_ARRAY

protected:
	HLSLCounterBufferBase() : counter(0) {}
	HLSLCounterBufferBase(uint capacity, uint counter = 0) : HLSLBufferBase(capacity), counter(counter) {}

	uint counter;
};

template<typename T> class RWBuffer : public HLSLBufferBase<T>
{
public:
	RWBuffer() {}
	RWBuffer(uint capacity) : HLSLBufferBase(capacity) {}

	oHLSL_OP_ARRAY
	oHLSL_CONST_OP_ARRAY
};

template<typename T> class StructuredBuffer : public HLSLBufferBase<T>
{
public:
	StructuredBuffer() {}
	StructuredBuffer(uint capacity) : HLSLBufferBase(capacity) {}

	oHLSL_CONST_OP_ARRAY
};

template<typename T> class RWStructuredBuffer : public HLSLCounterBufferBase<T>
{
public:
	RWStructuredBuffer() {}
	RWStructuredBuffer(uint capacity, uint counter = 0) : HLSLCounterBufferBase(capacity, counter) {}

	oHLSL_OP_ARRAY
	oHLSL_CONST_OP_ARRAY

	uint IncrementCounter() { uint orig = 0; InterlockedAdd(counter, 1, orig); return orig; }
	uint DecrementCounter() { uint orig = 0; InterlockedAdd(counter, -1, orig); return orig; }
};

template<typename T> class AppendStructuredBuffer : public HLSLCounterBufferBase<T>
{
public:
	AppendStructuredBuffer() {}
	AppendStructuredBuffer(uint capacity, uint counter = 0) : HLSLCounterBufferBase(capacity, counter) {}

	void Append(const T& a) { uint i = 0; InterlockedAdd(Counter, 1, i); (*this)[i] = a; }
};

template<typename T> class ConsumeStructuredBuffer : public HLSLCounterBufferBase<T>
{
public:
	ConsumeStructuredBuffer() {}
	ConsumeStructuredBuffer(uint capacity, uint counter = 0) : HLSLCounterBufferBase(capacity, counter) {}

	T& Consume() const { uint i = 0; InterlockedAdd(counter, -1, i); return (*this)[i-1]; }
};

template<typename T> struct ByteAddressBuffer
{
	inline void GetDimensions(uint& _Dimensions) const { _Dimensions = sizeof(T); }
	inline uint Load(uint addr) const { return *Ptr<uint>(addr); }
	inline uint2 Load2(uint addr) const { return *Ptr<uint2>(addr); }
	inline uint3 Load3(uint addr) const { return *Ptr<uint3>(addr); }
	inline uint4 Load4(uint addr) const { return *Ptr<uint4>(addr); }

protected:
  template<typename T> T aligned(T value, size_t alignment) { return value == (T)(((size_t)value + alignment - 1) & ~(alignment - 1)); }
	template<typename T> T* Ptr(uint addr) { assert(aligned(addr, 4) && "addr must be 4-byte aligned"); return (T*)((unsigned char*)this + addr); }
	template<typename T> const T* Ptr(uint addr) const { assert(aligned(addr, 4) && "addr must be 4-byte aligned"); return (const T*)((unsigned char*)this + addr); }
};

template<typename T> struct RWByteAddressBuffer : ByteAddressBuffer<T>
{
	inline void InterlockedAdd(uint dst, uint value, uint& orig) { ::InterlockedAdd(*Ptr<uint>(dst), value, orig); }
	inline void InterlockedAdd(uint dst, uint value) { ::InterlockedAdd(*Ptr<uint>(dst), value); }
	inline void InterlockedAnd(uint dst, uint value, uint& orig) { ::InterlockedAnd(*Ptr<uint>(dst), value, orig); }
	inline void InterlockedAnd(uint dst, uint value) { ::InterlockedAnd(*Ptr<uint>(dst), value); }
	inline void InterlockedCompareExchange(uint dst, uint compare_value, uint value, uint& orig) { ::InterlockedCompareExchange(*Ptr<uint>(dst), compare_value, value, orig); }
	inline void InterlockedCompareExchange(uint dst, uint compare_value, uint value) { ::InterlockedCompareExchange(*Ptr<uint>(dst), compare_value, value); }
	//inline void InterlockedCompareStore(uint dst, uint compare_value, uint value);
	inline void InterlockedExchange(uint dst, uint value, uint& orig) { ::InterlockedExchange(*Ptr<uint>(dst), value, orig); }
	inline void InterlockedExchange(uint dst, uint value) { ::InterlockedExchange(*Ptr<uint>(dst), value); }
	inline void InterlockedMax(uint dst, uint value, uint& orig) { ::InterlockedMax(*Ptr<uint>(dst), value, orig); }
	inline void InterlockedMax(uint dst, uint value) { ::InterlockedMax(*Ptr<uint>(dst), value); }
	inline void InterlockedMin(uint dst, uint value, uint& orig) { ::InterlockedMin(*Ptr<uint>(dst), value, orig); }
	inline void InterlockedMin(uint dst, uint value) { ::InterlockedMin(*Ptr<uint>(dst), value); }
	inline void InterlockedOr(uint dst, uint value, uint& orig) { ::InterlockedOr(*Ptr<uint>(dst), value, orig); }
	inline void InterlockedOr(uint dst, uint value) { ::InterlockedOr(*Ptr<uint>(dst), value); }
	inline void InterlockedXor(uint dst, uint value, uint& orig) { ::InterlockedXor(*Ptr<uint>(dst), value, orig); }
	inline void InterlockedXor(uint dst, uint value) { ::InterlockedXor(*Ptr<uint>(dst), value); }

	inline void Store(uint addr, uint value) { InterlockedExchange(addr, value); }
	inline void Store2(uint addr, const uint2& value) { Store(addr, value.x); Store(addr + 4, value.y); }
	inline void Store3(uint addr, const uint3& value) { Store(addr, value.x); Store(addr + 4, value.y); Store(addr + 8, value.z); }
	inline void Store4(uint addr, const uint4& value) { Store(addr, value.x); Store(addr + 4, value.y); Store(addr + 8, value.z); Store(addr + 12, value.w); }
};

#undef oHLSL_OP_ARRAY
#undef oHLSL_CONST_OP_ARRAY

// _____________________________________________________________________________
// Unimplemented

// This tends to be unimplemented in HW. Use a software simplex/perlin noise 
// implementation so the math is know to behave the same in C++ and HLSL.
//template<typename T, typename TVec> T noise(const TVec& x);

// Partial derivatives. It would require significant non-math infrastructure to 
// support these functions, so they are unimplemented.
//template<typename T> T ddx(const T& x);
//template<typename T> T ddx_coarse(const T& x);
//template<typename T> T ddx_fine(const T& x);
//template<typename T> T ddy(const T& x);
//template<typename T> T ddy_coarse(const T& x);
//template<typename T> T ddy_fine(const T& x);
//template<typename T> T fwidth(const T& x) { return abs(ddx(x)) + abs(ddy(x)); }
//D3DCOLORtoUBYTE4
//dst

// Interop/keyword wrappers
#define oIN(Type, Param) const Type& Param
#define oOUT(Type, Param) Type& Param
#define oINOUT(Type, Param) Type& Param
#define oHLSL_UNIFORM
#define oHLSL_ALLOW_UAV_CONDITION
#define oHLSL_LOOP
#define oHLSL_UNROLL
#define oHLSL_UNROLL1(x)
#define oHLSL_FASTOP
#define oHLSL_BRANCH
#define oHLSL_CONSTANT_BUFFER(slot)
#define oHLSL_RESOURCE_BUFFER(slot)
#define oHLSL_UNORDERED_BUFFER(slot)

// _____________________________________________________________________________
// ouro::equal support

#if oHLSL_OURO_EQUAL_SUPPORT == 1

#include <oMath/equal.h>
#include <oMath/equal_eps.h>

namespace ouro {
template<> inline bool equal(const oHLSL2<float>&    a, const oHLSL2<float>&    b, unsigned int max_ulps) { return equal(a.x, b.x, max_ulps) && equal(a.y, b.y, max_ulps); }
template<> inline bool equal(const oHLSL3<float>&    a, const oHLSL3<float>&    b, unsigned int max_ulps) { return equal(a.x, b.x, max_ulps) && equal(a.y, b.y, max_ulps) && equal(a.z, b.z, max_ulps); }
template<> inline bool equal(const oHLSL4<float>&    a, const oHLSL4<float>&    b, unsigned int max_ulps) { return equal(a.x, b.x, max_ulps) && equal(a.y, b.y, max_ulps) && equal(a.z, b.z, max_ulps) && equal(a.w, b.w, max_ulps); }
template<> inline bool equal(const oHLSL3x3<float>&  a, const oHLSL3x3<float>&  b, unsigned int max_ulps) { return equal(a.col0, b.col0, max_ulps) && equal(a.col1, b.col1, max_ulps) && equal(a.col2, b.col2, max_ulps); }
template<> inline bool equal(const oHLSL4x4<float>&  a, const oHLSL4x4<float>&  b, unsigned int max_ulps) { return equal(a.col0, b.col0, max_ulps) && equal(a.col1, b.col1, max_ulps) && equal(a.col2, b.col2, max_ulps) && equal(a.col3, b.col3, max_ulps); }
template<> inline bool equal(const oHLSL2<double>&   a, const oHLSL2<double>&   b, unsigned int max_ulps) { return equal(a.x, b.x, max_ulps) && equal(a.y, b.y, max_ulps); }
template<> inline bool equal(const oHLSL3<double>&   a, const oHLSL3<double>&   b, unsigned int max_ulps) { return equal(a.x, b.x, max_ulps) && equal(a.y, b.y, max_ulps) && equal(a.z, b.z, max_ulps); }
template<> inline bool equal(const oHLSL4<double>&   a, const oHLSL4<double>&   b, unsigned int max_ulps) { return equal(a.x, b.x, max_ulps) && equal(a.y, b.y, max_ulps) && equal(a.z, b.z, max_ulps) && equal(a.w, b.w, max_ulps); }
template<> inline bool equal(const oHLSL3x3<double>& a, const oHLSL3x3<double>& b, unsigned int max_ulps) { return equal(a.col0, b.col0, max_ulps) && equal(a.col1, b.col1, max_ulps) && equal(a.col2, b.col2, max_ulps); }
template<> inline bool equal(const oHLSL4x4<double>& a, const oHLSL4x4<double>& b, unsigned int max_ulps) { return equal(a.col0, b.col0, max_ulps) && equal(a.col1, b.col1, max_ulps) && equal(a.col2, b.col2, max_ulps) && equal(a.col3, b.col3, max_ulps); }

template<> inline bool equal_eps(const oHLSL2<float>&    a, const oHLSL2<float>&    b, float eps)  { return equal_eps(a.x, b.x, eps) && equal_eps(a.y, b.y, eps); }
template<> inline bool equal_eps(const oHLSL3<float>&    a, const oHLSL3<float>&    b, float eps)  { return equal_eps(a.x, b.x, eps) && equal_eps(a.y, b.y, eps); }
template<> inline bool equal_eps(const oHLSL4<float>&    a, const oHLSL4<float>&    b, float eps)  { return equal_eps(a.x, b.x, eps) && equal_eps(a.y, b.y, eps); }
template<> inline bool equal_eps(const oHLSL3x3<float>&  a, const oHLSL3x3<float>&  b, float eps)  { return equal_eps(a.col0, b.col0, eps) && equal_eps(a.col1, b.col1, eps) && equal_eps(a.col2, b.col2, eps); }
template<> inline bool equal_eps(const oHLSL4x4<float>&  a, const oHLSL4x4<float>&  b, float eps)  { return equal_eps(a.col0, b.col0, eps) && equal_eps(a.col1, b.col1, eps) && equal_eps(a.col2, b.col2, eps) && equal_eps(a.col3, b.col3, eps); }
template<> inline bool equal_eps(const oHLSL2<double>&   a, const oHLSL2<double>&   b, double eps) { return equal_eps(a.x, b.x, eps) && equal_eps(a.y, b.y, eps); }
template<> inline bool equal_eps(const oHLSL3<double>&   a, const oHLSL3<double>&   b, double eps) { return equal_eps(a.x, b.x, eps) && equal_eps(a.y, b.y, eps); }
template<> inline bool equal_eps(const oHLSL4<double>&   a, const oHLSL4<double>&   b, double eps) { return equal_eps(a.x, b.x, eps) && equal_eps(a.y, b.y, eps); }
template<> inline bool equal_eps(const oHLSL3x3<double>& a, const oHLSL3x3<double>& b, double eps) { return equal_eps(a.col0, b.col0, eps) && equal_eps(a.col1, b.col1, eps) && equal_eps(a.col2, b.col2, eps); }
template<> inline bool equal_eps(const oHLSL4x4<double>& a, const oHLSL4x4<double>& b, double eps) { return equal_eps(a.col0, b.col0, eps) && equal_eps(a.col1, b.col1, eps) && equal_eps(a.col2, b.col2, eps) && equal_eps(a.col3, b.col3, eps); }
}
#endif

#else

// Interop/keyword wrappers
#define oIN(Type, Param) in Type Param
#define oOUT(Type, Param) out Type Param
#define oINOUT(Type, Param) inout Type Param
#define oHLSL_UNIFORM uniform
#define oHLSL_ALLOW_UAV_CONDITION [allow_uav_condition]
#define oHLSL_LOOP [loop]
#define oHLSL_UNROLL [unroll]
#define oHLSL_UNROLL1(x) [unroll(x)]
#define oHLSL_FASTOPT [fastopt]
#define oHLSL_BRANCH [branch]
#define oHLSL_CONSTANT_BUFFER(slot) register(c##slot)
#define oHLSL_RESOURCE_BUFFER(slot) register(t##slot)
#define oHLSL_UNORDERED_BUFFER(slot) register(u##slot)

#endif
#endif