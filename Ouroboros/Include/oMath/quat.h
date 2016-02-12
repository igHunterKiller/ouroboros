// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// utilities for working with quaternions
// http://code.google.com/p/kri/wiki/quats
// normalize a quat just like a regular float4

#ifndef oHLSL
	#pragma once
#endif
#ifndef oMath_quat_h
#define oMath_quat_h

#include <oMath/hlsl.h>

#ifndef oHLSL
#include <oMath/hlsl_swizzles_on.h>
#endif

// return vector v rotated quaternion q
inline float3 qmul(oIN(float4, q), oIN(float3, v))
{
	// http://code.google.com/p/kri/wiki/quats
	#if 1
		return v + 2.0f*cross(q.xyz, cross(q.xyz,v) + q.w*v);
	#else
		return v*(q.w*q.w - dot(q.xyz,q.xyz)) + 2.0f*q.xyz*dot(q.xyz,v) + 2.0f*q.w*cross(q.xyz,v);
	#endif
}

// returns a*b of two quats (non communicative)
inline float4 qmul(oIN(float4, a), oIN(float4, b)) { return float4(cross(a.xyz,b.xyz) + a.xyz*b.w + b.xyz*a.w, a.w*b.w - dot(a.xyz,b.xyz)); }
inline float4 conjugate(oIN(float4, a)) { return float4(-a.x, -a.y, -a.z, a.w); }
inline float4 invert(oIN(float4, a)) { float4 v = conjugate(a).xyzw * (1.0f / dot(a.xyzw, a.xyzw)); return v; }

// returns the quaternion describing the rotation from orientation a to b
inline float4 qfromto(oIN(float4, from), oIN(float4, to)) { return from * conjugate(to); }

// returns a rotation matrix from a quaternion
inline float3x3 qrotation(oIN(float4, q))
{
	float3x3 r;
  r[0] = float3(1.0f,0.0f,0.0f) + float3(-2.0f,2.0f,2.0f)*q.y*q.yxw + float3(-2.0f,-2.0f,2.0f)*q.z*q.zwx;
  r[1] = float3(0.0f,1.0f,0.0f) + float3(2.0f,-2.0f,2.0f)*q.z*q.wzy + float3(2.0f,-2.0f,-2.0f)*q.x*q.yxw;
  r[2] = float3(0.0f,0.0f,1.0f) + float3(2.0f,2.0f,-2.0f)*q.x*q.zwx + float3(-2.0f,2.0f,-2.0f)*q.y*q.wzy;
	return r;
}

// a and b must be normalized quaternions
inline float4 slerp(oIN(float4, a), oIN(float4, b), float s)
{
	// Erwin Coumans, Zlib license
	// http://continuousphysics.com/Bullet/
	#define _VECTORMATH_SLERP_TOL 0.999f
	float4 start;
	float recipSinAngle, scale0, scale1, cosAngle, angle;
	cosAngle = dot( a.xyzw, b.xyzw );
	if ( cosAngle < 0.0f ) {
		cosAngle = -cosAngle;
		start = ( -a );
	} else {
		start = a;
	}
	if ( cosAngle < _VECTORMATH_SLERP_TOL ) {
		angle = acos( cosAngle );
		recipSinAngle = ( 1.0f / sin( angle ) );
		scale0 = ( sin( ( ( 1.0f - s ) * angle ) ) * recipSinAngle );
		scale1 = ( sin( ( s * angle ) ) * recipSinAngle );
	} else {
		scale0 = ( 1.0f - s );
		scale1 = s;
	}
	return ( ( start * scale0 ) + ( b * scale1 ) );
}

#ifndef oHLSL
#include <oMath/hlsl_swizzles_off.h>
#endif

#endif
