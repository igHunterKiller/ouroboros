// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// utilities for working with perspective and orthographic proj matrices
//
// describes LH and RH perspective matrices right next to each other.
// http://knol.google.com/k/perspective-transformation
//
// Infinite projection matrix
// http://www.google.com/url?sa=t&source=web&cd=2&ved=0CBsQFjAB&url=http%3A%2F%2Fwww.terathon.com%2Fgdc07_lengyel.ppt&rct=j&q=eric%20lengyel%20projection&ei=-NpaTZvWKYLCsAOluNisCg&usg=AFQjCNGkbo93tbmlrXqkbdJg-krdEYNS1A
//
// Exhaustive Foundational Information
// http://scratchapixel.com/index.php?nopage

// linear depth: view-space depth that's in world-space units relative to the eye positioned at the origin.
// hyperbolic depth: clip-space depth, or view-space depth run through a projection matrix.
// post-projection depth: clip-space depth / clip-space w. (Z/W)

#pragma once
#include <oMath/hlsl.h>
#include <oMath/floats.h>
#include <oMath/equal.h>
#include <array>

namespace ouro {

// indexed by values in hexahedron.h: left right top bottom near far
typedef std::array<float4, 6> frustum_t;

// _____________________________________________________________________________
// Direct projection matrix construction

// if far is 0 or less then an infinite far clip plane is used
float4x4 proj_fovx_lh(float fovx_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 proj_fovy_lh(float fovy_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 proj_fovx_rh(float fovx_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 proj_fovy_rh(float fovy_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);

// _____________________________________________________________________________
// Direct inverse projection matrix construction. Prefer this to using invert()
// on a projection matrix to minimize operations and thus maximize precision.

float4x4 proj_inv_fovx_lh(float fovx_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 proj_inv_fovy_lh(float fovy_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 proj_inv_fovx_rh(float fovx_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 proj_inv_fovy_rh(float fovy_radians, float aspect_ratio, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);

// _____________________________________________________________________________
// Lossless extraction of projection parameters (no precision concerns)

inline bool has_projection(const float4x4& proj) { return (proj[0][3] != 0.0f || proj[1][3] != 0.0f || proj[2][3] != 0.0f); }
inline bool is_left_handed(const float4x4& proj) { return proj[2][3] > 0.0f; }
inline bool is_right_handed(const float4x4& proj) { return proj[2][3] < 0.0f; }

// returns the values to pass to the hyper <-> lin depth conversions below
// LH: x =  f / (f-n); y = -n * f / (f-n) (or if _far < 0, an infinite far clip plane)
// RH: x = -f / (f-n); y = -n * f / (f-n) (or if _far < 0, an infinite far clip plane)
float2 depth_constants_lh(float near, float far);
float2 depth_constants_rh(float near, float far);

// _____________________________________________________________________________
// Potentially lossy extraction of projection parameters. Prefer storing such
// values and using them rather than decomposing the projection for them.

inline float2 proj_depth_constants(const float4x4& proj) { return float2(proj[2][2], proj[3][2]); }
inline float  proj_fovx           (const float4x4& proj) { return (has_projection(proj) ? 2.0f : 1.0f) * atan(1.0f / proj[0][0]); }
inline float  proj_fovy           (const float4x4& proj) { return (has_projection(proj) ? 2.0f : 1.0f) * atan(1.0f / proj[1][1]); }
inline float2 proj_fov            (const float4x4& proj) { return float2(proj_fovx(proj), proj_fovy(proj)); }
inline float2 proj_ratio          (const float4x4& proj) { return 1.0f / float2(proj[0][0], proj[1][1]); }
inline float  proj_aspect_ratio   (const float4x4& proj) { return proj[1][1] / proj[0][0]; }
       float  proj_near           (const float4x4& proj);
       float  proj_far            (const float4x4& proj);

// _____________________________________________________________________________
// Depth space conversion

// This same math can be used in a pixel shader to take a depth-buffer sample (hyperbolic) and convert it to linear depth
// linear depth is in view-space units (i.e. eye.z + linear_depth is world-space z)
inline float hyperbolic_to_linear_depth(const float2& depth_constants, float hyperbolic_depth) { return depth_constants.y / (depth_constants.x - hyperbolic_depth); }
inline float linear_to_hyperbolic_depth(const float2& depth_constants, float VSdepth)          { return depth_constants.x + (depth_constants.y /          VSdepth); }

// _____________________________________________________________________________
// Orthographic projections (API above this not tested with ortho matrices)

float4x4 ortho_offcenter_lh(float left, float right, float bottom, float top, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 ortho_offcenter_rh(float left, float right, float bottom, float top, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);

float4x4 ortho_lh(float width, float height, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);
float4x4 ortho_rh(float width, float height, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);

// _____________________________________________________________________________
// Offcenter projection (API above this not tested with offcenter matrices)

// create an off-axis (off-center) proj matrix (or a centered one if left/right and bottom/top are balanced)
float4x4 proj_offcenter_lh(float left, float right, float bottom, float top, float near = oDEFAULT_NEAR_CLIPf, float far = oDEFAULT_FAR_CLIPf);

// create an off-axis (off-center) proj matrix properly sized to 
// display_transform (i.e. the screen if it were transformed in world space)

// output_transform: Scale, Rotation, Translation (SRT) of the output device 
// plane (i.e. the screen) in the same space as the eye (usually world space)
// eye: the location of the eye in the same space as display_transform
// near: A near plane that is different than the display plane (physical glass 
// of the screen). By specifying a near plane that is closer to the eye than 
// the glass plane, an effect of "popping out" 3D can be achieved.
float4x4 proj_offcenter_lh(const float4x4& display_transform, const float3& eye, float near = oDEFAULT_NEAR_CLIPf);

// _____________________________________________________________________________
// Projection matrix geometry: frustum planes and points

// decomposes the projection matrix into planes fit for frustum culling in oHEX order
frustum_t proj_frustum(const float4x4& proj, bool normalize_planes = true);

// returns true if out_corners contains the specified frustum's 8 corner points
// or false of the frustum is poorly formed.
bool proj_corners(const frustum_t& frust, float3 out_corners[8]);
inline bool proj_corners(const float4x4& proj, float3 out_corners[8]) { return proj_corners(proj_frustum(proj, true), out_corners); }

inline bool equal(const frustum_t& a, const frustum_t& b, unsigned int max_ulps = oDEFAULT_ULPS)
{
	for (auto x = a.cbegin(), y = b.cbegin(); x != a.cend(); ++x, ++y)
		if (!equal(*x, *y, max_ulps))
			return false;
	return true;
}

// _____________________________________________________________________________
// Utility

// Return the view-space position given a screen position (in pixels where 0,0 is upper left) and 
// view-space depth (world-space where the eye is at origin) along with viewport parameters and 
// projection matrix.
float3 unproject(const float2& screen_position, float VSdepth, const float2& viewport_position, const float2& viewport_dimensions, const float4x4& proj);

}
