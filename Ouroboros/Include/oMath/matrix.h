// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// utilities for working with column-major, affine matrices
// see projection.h for projection-specific api

#pragma once
#include <oMath/hlsl.h>

namespace ouro {

// _____________________________________________________________________________
// General

inline bool     affine               (const float4x4& m)                                    { return equal(float4(0.0f, 0.0f, 0.0f, 1.0f), float4(m[0].w, m[1].w, m[2].w, m[3].w)); }
       float4x4 invert               (const float4x4& m);
       float3x3 invert               (const float3x3& m);

// Decomposes m into components as if they were applied in order:
// scaleXYZ shearXY shearXZ shearZY rotationXYZ translationXYZ projectionXYZW
// Returns false if m is singular. Note: This does not support negative scale 
// well: rotations might appear rotated by 180 degrees and the resulting scale 
// can be the wrong sign.
bool decompose                       (const float4x4& m, float3* out_scale, float3* out_shear, float3* out_rotation, float3* out_translation, float4* out_perspective);

// _____________________________________________________________________________
// Scale

inline float4x4 scale                (const float3& s)
{
	return float4x4(
		float4( s.x, 0.0f, 0.0f, 0.0f),
		float4(0.0f,  s.y, 0.0f, 0.0f),
		float4(0.0f, 0.0f,  s.z, 0.0f),
		float4(0.0f, 0.0f, 0.0f, 1.0f));
}

inline float4x4 scale                (float s)                                              { return scale(float3(s)); }
inline float3   scale                (const float3x3& m)                                    { return float3(length(m[0]), length(m[1]), length(m[2])); }
inline float3   scale                (const float4x4& m)                                    { return float3(length(m[0].xyz()), length(m[1].xyz()), length(m[2].xyz())); }
inline float3x3 rescale              (const float3x3& m, float new_scale)                   { return float3x3(normalize(m[0]) * new_scale, normalize(m[1]) * new_scale, normalize(m[2]) * new_scale); }
inline float4x4 rescale              (const float4x4& m, float new_scale)                   { return float4x4(normalize(m[0]) * new_scale, normalize(m[1]) * new_scale, normalize(m[2]) * new_scale, m[3]); }
inline float3x3 remove_scale         (const float3x3& m)                                    { return float3x3(normalize(m[0]), normalize(m[1]), normalize(m[2])); }
inline float3x4 remove_scale         (const float3x4& m)                                    { return float3x4(normalize(m[0]), normalize(m[1]), normalize(m[2])); }
inline float4x4 remove_scale         (const float4x4& m)                                    { return float4x4(normalize(m[0]), normalize(m[1]), normalize(m[2]), m[3]); }
       float3x3 orthonormalize       (const float3x3& m);
inline float4x4 orthonormalize       (const float4x4& m)                                    { return float4x4(orthonormalize((float3x3)m), m[3].xyz()); }
       float4x4 normalization        (const float3& aabb_min, const float3& aabb_max); // matrix to scale & translate to fit inside an aabb

// _____________________________________________________________________________
// Shear

       float4x4 remove_shear         (const float4x4& m);
inline float3x3 remove_shear         (const float3x3& m)                                    { return (float3x3)remove_shear(float4x4(m)); }
       float4x4 remove_scale_shear   (const float4x4& m);
inline float3x3 remove_scale_shear   (const float3x3& m)                                    { return (float3x3)remove_scale_shear(float4x4(m)); }

// _____________________________________________________________________________
// Rotation

inline float4x4 rotatex              (float radians)
{
	float s, c;
	sincos(radians, s, c);
	return float4x4(
		float4(1.0f, 0.0f,  0.0f, 0.0f),
		float4(0.0f,    c,    -s, 0.0f),
		float4(0.0f,    s,     c, 0.0f),
		float4(0.0f, 0.0f,  0.0f, 1.0f));
}

inline float4x4 rotatey              (float radians)
{
	float s, c;
	sincos(radians, s, c);
	return float4x4(
		float4(    c, 0.0f,    s, 0.0f),
		float4( 0.0f, 1.0f, 0.0f, 0.0f),
		float4(   -s, 0.0f,    c, 0.0f),
		float4( 0.0f, 0.0f, 0.0f, 1.0f));
}

inline float4x4 rotatez              (float radians)
{
	float s, c;
	sincos(radians, s, c);
	return float4x4(
		float4(   c,    -s, 0.0f, 0.0f),
		float4(   s,     c, 0.0f, 0.0f),
		float4(0.0f,  0.0f, 1.0f, 0.0f),
		float4(0.0f,  0.0f, 0.0f, 1.0f));
}

inline float4x4 rotate               (const float3& radians)
{
	float3 s, c;
	sincos(radians, s, c);
	const float i = c.z * s.y;
	const float j = s.z * s.y;
	return float4x4(
		float4(           c.z * c.y,           s.z * c.y,      -s.y, 0.0f ),
		float4( i * s.x - s.z * c.x, j * s.x + c.z * c.x, c.y * s.x, 0.0f ),
		float4( i * c.x + s.z * s.x, j * c.x - c.z * s.x, c.y * c.x, 0.0f ),
		float4(                0.0f,                0.0f,      0.0f, 1.0f));
}

       float4x4 rotate               (float radians, const float3& normalized_rotation_axis);
       float4x4 rotate               (const float3& normalized_from, const float3& normalized_to);
       float4x4 rotate               (const float4& quaternion);
       float4x4 rotate_xy_planar     (const float4& normalized_plane); // rotates from XY plane, +Z up to points on normalized_plane
       float3   rotation             (const float4x4& m);
inline float3   rotation             (const float3x3& m)                                    { return rotation(float4x4(m)); }
inline float3   rotation2            (const float4x4& m)                                    { return float3(atan2(m[3][2], m[3][3]), atan2(-m[3][1], sqrt(m[3][2] * m[3][2] + m[3][3] * m[3][3])), atan2(m[2][1], m[1][1])); }
inline float4x4 remove_rotation      (const float4x4& m)                                    { float4x4 rr = scale(scale(m)); rr[3] = m[3]; return rr; }
       float4   qrotate              (const float3& radians);                                      // returns normalized quaternion (rotation only)
       float4   qrotate              (float radians, const float3& normalized_rotation_axis);      //  "
       float4   qrotate              (const float3& normalized_from, const float3& normalized_to); //  " note: result undefined when vectors point in opposite directions
       float4   qrotate              (const float3x3& m);                                          //  "
       float4   qrotate              (const float4x4& m);                                          //  "
inline float4x4 rotatetranslate      (const float3& radians, const float3& translation)    { return float4x4(rotate(radians), translation); }
inline float4x4 qrotatetranslate     (const float4& quaternion, const float3& translation) { return float4x4((float3x3)rotate(quaternion), translation); }
inline float3   quattoeuler          (const float4& quaternion)                            { return rotation(rotate(quaternion)); }


// _____________________________________________________________________________
// Translation

inline float4x4 translate            (const float3& translation)
{
	return float4x4(
		float4(1.0f, 0.0f, 0.0f, 0.0f),
		float4(0.0f, 1.0f, 0.0f, 0.0f),
		float4(0.0f, 0.0f, 1.0f, 0.0f),
		float4(   translation,   1.0f));
}

inline float3   translation          (const float4x4& m)                                   { return m[3].xyz(); }

// _____________________________________________________________________________
// Reflection

inline float4x4 reflect              (const float4& normalized_reflection_plane)
{
	const float4 n = normalized_reflection_plane;
	return float4x4(
    float4(2.0f * n.x * n.x + 1.0f, 2.0f * n.y * n.x,        2.0f * n.z * n.x,        0.0f),
    float4(2.0f * n.x * n.y,        2.0f * n.y * n.y + 1.0f, 2.0f * n.z * n.y,        0.0f),
    float4(2.0f * n.x * n.z,        2.0f * n.y * n.z,        2.0f * n.z * n.z + 1.0f, 0.0f),
    float4(2.0f * n.x * n.w,        2.0f * n.y * n.w,        2.0f * n.z * n.w,        1.0f));
}

// _____________________________________________________________________________
// View (world-to-view)

       float4x4 lookat_lh            (const float3& eye, const float3& at, const float3& up);
       float4x4 lookat_rh            (const float3& eye, const float3& at, const float3& up);
       float4x4 translate_view_to_fit(const float4x4& view, float fovy_radians, const float4& sphere);
inline float3   view_eye             (const float4x4& view)                                { float4x4 m = invert(view); return translation(m); }
inline float4x4 flip_view_handedness (const float4x4& view)                                { float4x4 m = view; m[2] = -m[2]; return m; } // rh <-> lh

}
