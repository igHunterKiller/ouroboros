// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// coordinate system conversions

#ifndef oHLSL
#pragma once
#endif
#ifndef oMath_coord_h
#define oMath_coord_h

#include <oMath/hlsl.h>
#include <oMath/hlslx.h>

inline float3 cartesian_to_spherical(oIN(float3, p))
{
  const float r = length(p);
  const float theta = atan(p.y / max(p.x, oVERY_SMALLf));
  const float phi = acos(p.z / max(r, oVERY_SMALLf));
  return float3(r, theta, phi);
}

inline float3 spherical_to_cartesian(oIN(float3, p))
{
  const float r = p.x;
  const float theta = p.y;
  const float phi = p.z;
  const float r_sin_phi = r * sin(phi);
  return float3(cos(theta) * r_sin_phi, sin(theta) * r_sin_phi, r * cos(phi));
}

inline float3 cartesian_to_cylindrical(oIN(float3, p))
{
  const float r = length(p.xy);
  const float theta = atan(p.y / max(p.x, oVERY_SMALLf));
  return float3(r, theta, p.z);
}

inline float3 cylindrical_to_cartesian(oIN(float3, p))
{
  const float r = p.x;
  const float theta = p.y;
  return float3(r * cos(theta), r * sin(theta), p.z);
}

// laea: Lambert Azimuthal Equal-Area projection
// assumes n is a normalized normal/vector
inline float2 cartesian_to_laea(oIN(float3, n))
{
	const float2 x = normalize(n.xy) * sqrt(n.z * 0.5f + 0.5f);
	return x * 0.5f + 0.5f;
}

inline float3 laea_to_cartesian(oIN(float2, x))
{
	const float2 tmp = x * 4.0f + 2.0f;
	const float f = dot(tmp, tmp);
	const float g = sqrt(1 - f / 4.0f);
	return float3(tmp.x * g, tmp.y * g, 1.0f - f / 2.0f);
}

#endif
