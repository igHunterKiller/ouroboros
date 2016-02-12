// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/projection.h>
#include <oMath/hlslx.h>
#include <oMath/cube_topology.h>
#include <oMath/matrix.h>
#include <oMath/plane_v_plane_v_plane.h>

namespace ouro {

static float2 scale_constants_fovy(float fovy_radians, float aspect_ratio)
{
	const float sy = 1.0f / tan(fovy_radians * 0.5f);
	return float2(sy / aspect_ratio, sy);
}

static float2 scale_constants_fovx(float fovx_radians, float aspect_ratio)
{
	const float sx = 1.0f / tan(fovx_radians * 0.5f);
	return float2(sx, sx * aspect_ratio);
}

static float2 scale_rcp_constants_fovy(float fovy_radians, float aspect_ratio)
{
	const double fy = fovy_radians;
	const double ar = aspect_ratio;
	const double sy_rcp = tan(fy * 0.5);
	const double sx_rcp = sy_rcp * aspect_ratio;
	return float2((float)sx_rcp, (float)sy_rcp);
}

static float2 scale_rcp_constants_fovx(float fovx_radians, float aspect_ratio)
{
	const double fx = fovx_radians;
	const double ar = aspect_ratio;
	const double sx_rcp = tan(fx * 0.5);
	const double sy_rcp = sx_rcp / aspect_ratio;
	return float2((float)sx_rcp, (float)sy_rcp);
}

float4x4 proj_fovy_lh(float fovy_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_constants_fovy(fovy_radians, aspect_ratio);
	const float2 d = depth_constants_lh(near, far);
	return float4x4(
		float4(s.x,  0.0f, 0.0f, 0.0f),
		float4(0.0f, s.y,  0.0f, 0.0f),
		float4(0.0f, 0.0f, d.x,  1.0f),
		float4(0.0f, 0.0f, d.y,  0.0f));
}

float4x4 proj_fovy_rh(float fovy_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_constants_fovy(fovy_radians, aspect_ratio);
	const float2 d = depth_constants_rh(near, far);
	return float4x4(
		float4(s.x,  0.0f, 0.0f, 0.0f),
		float4(0.0f, s.y,  0.0f, 0.0f),
		float4(0.0f, 0.0f, d.x, -1.0f),
		float4(0.0f, 0.0f, d.y,  0.0f));
}

float4x4 proj_fovx_lh(float fovx_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_constants_fovx(fovx_radians, aspect_ratio);
	const float2 d = depth_constants_lh(near, far);
	return float4x4(
		float4(s.x,  0.0f, 0.0f, 0.0f),
		float4(0.0f, s.y,  0.0f, 0.0f),
		float4(0.0f, 0.0f, d.x,  1.0f),
		float4(0.0f, 0.0f, d.y,  0.0f));
}

float4x4 proj_fovx_rh(float fovx_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_constants_fovx(fovx_radians, aspect_ratio);
	const float2 d = depth_constants_rh(near, far);
	return float4x4(
		float4(s.x,  0.0f, 0.0f, 0.0f),
		float4(0.0f, s.y,  0.0f, 0.0f),
		float4(0.0f, 0.0f, d.x, -1.0f),
		float4(0.0f, 0.0f, d.y,  0.0f));
}

static float2 proj_inv_depth_constants_lh(float near, float far)
{
	const double dn = near;
	const double df = far;
	const double near_rcp = 1.0 / dn;
	const double d_y_rcp = (df - dn) / (-dn * df);
	return float2((float)near_rcp, (float)d_y_rcp);
}

static float2 proj_inv_depth_constants_rh(float near, float far)
{
	return proj_inv_depth_constants_lh(near, far);
}

float4x4 proj_inv_fovy_lh(float fovy_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_rcp_constants_fovy(fovy_radians, aspect_ratio);
	const float2 d = proj_inv_depth_constants_lh(near, far);
	return float4x4(
		float4(s.x,  0.0f, 0.0f, 0.0f),
		float4(0.0f, s.y,  0.0f, 0.0f),
		float4(0.0f, 0.0f, 0.0f, d.y),
		float4(0.0f, 0.0f, 1.0f, d.x));
}

float4x4 proj_inv_fovx_lh(float fovx_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_rcp_constants_fovx(fovx_radians, aspect_ratio);
	const float2 d = proj_inv_depth_constants_lh(near, far);
	return float4x4(
		float4(s.x,  0.0f, 0.0f, 0.0f),
		float4(0.0f, s.y,  0.0f, 0.0f),
		float4(0.0f, 0.0f, 0.0f, d.y),
		float4(0.0f, 0.0f, 1.0f, d.x));
}

float4x4 proj_inv_fovy_rh(float fovy_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_rcp_constants_fovy(fovy_radians, aspect_ratio);
	const float2 d = proj_inv_depth_constants_rh(near, far);
	return float4x4(
		float4(s.x,  0.0f,  0.0f, 0.0f),
		float4(0.0f, s.y,   0.0f, 0.0f),
		float4(0.0f, 0.0f,  0.0f, d.y),
		float4(0.0f, 0.0f, -1.0f, d.x));
}

float4x4 proj_inv_fovx_rh(float fovx_radians, float aspect_ratio, float near, float far)
{
	const float2 s = scale_rcp_constants_fovx(fovx_radians, aspect_ratio);
	const float2 d = proj_inv_depth_constants_rh(near, far);
	return float4x4(
		float4(s.x,  0.0f,  0.0f, 0.0f),
		float4(0.0f, s.y,   0.0f, 0.0f),
		float4(0.0f, 0.0f,  0.0f, d.y),
		float4(0.0f, 0.0f, -1.0f, d.x));
}

float2 depth_constants_lh(float near, float far)
{
	static const float kZPrecision = 0.0001f;
	if (far < 0.0f)
		return float2(1.0f - kZPrecision, near * (2.0f - kZPrecision));
	double denom = far - near;
	double q = far / denom;
	double r = -near * far / denom;
	return float2((float)q, (float)r);
}

float2 depth_constants_rh(float near, float far)
{
	static const float kZPrecision = 0.0001f;
	if (far < 0.0f)
		return float2(1.0f - kZPrecision, near * (-2.0f - kZPrecision));
	double denom = far - near;
	double q = -far / denom;
	double r = -near * far / denom;
	return float2((float)q, (float)r);
}

float proj_near(const float4x4& proj)
{
	const float2 d = proj_depth_constants(proj);
	const float n = (equal(d.x, 0.0f)) ? 0.0f : (-d.y / d.x);
	return (proj[2][3] < 0.0f || d.x < 0.0f) ? -n : n;
}

float proj_far(const float4x4& proj)
{
	float2 d = proj_depth_constants(proj);
	const float n = proj_near(proj);
	if (has_projection(proj))
	{
		d.x *= proj[2][3];
		return d.x * n / (d.x - 1.0f);
	}
	return (d.x < 0.0f ? -1.0f : 1.0f) * (1.0f - d.y) / d.x;
}

float4x4 ortho_offcenter_lh(float left, float right, float bottom, float top, float near, float far)
{
	return float4x4(
    float4(2.0f / (right-left),         0.0f,                        0.0f,              0.0f),
    float4(0.0f,                        2.0f / (top-bottom),         0.0f,              0.0f),
    float4(0.0f,                        0.0f,                        1.0f / (far-near), 0.0f),
    float4((left+right) / (left-right), (top+bottom) / (bottom-top), near / (near-far), 1.0f));
}

float4x4 ortho_offcenter_rh(float left, float right, float bottom, float top, float near, float far)
{
	return float4x4(
    float4(2.0f / (right-left),         0.0f,                        0.0f,              0.0f),
    float4(0.0f,                        2.0f / (top-bottom),         0.0f,              0.0f),
    float4(0.0f,                        0.0f,                        1.0f / (near-far), 0.0f),
    float4((left+right) / (left-right), (top+bottom) / (bottom-top), near / (near-far), 1.0f));
}

float4x4 ortho_lh(float width, float height, float near, float far)
{
	return float4x4(
    float4(2.0f / width, 0.0f,          0.0f,              0.0f),
    float4(0.0f,         2.0f / height, 0.0f,              0.0f),
    float4(0.0f,         0.0f,          1.0f / (far-near), 0.0f),
    float4(0.0f,         0.0f,          near / (near-far), 1.0f));
}

float4x4 ortho_rh(float width, float height, float near, float far)
{
	return float4x4(
    float4(2.0f / width, 0.0f,          0.0f,              0.0f),
    float4(0.0f,         2.0f / height, 0.0f,              0.0f),
    float4(0.0f,         0.0f,          1.0f / (near-far), 0.0f),
    float4(0.0f,         0.0f,          near / (near-far), 1.0f));
}

static float2 depth_constants_offcenter_lh(float near, float far)
{
	// @tony why is this all that differnt from the regular path?
	// investigate further when I have a test case again

	static const float kZPrecision = 0.0001f;
	if (far < 0.0f)
		return float2(1.0f - kZPrecision, near * (-2.0f - kZPrecision));
	double denom = far - near;
	double q = far / denom;
	double r = near * far / denom;
	return float2((float)q, (float)r);
}

float4x4 proj_offcenter_lh(float left, float right, float bottom, float top, float near, float far)
{
	float2 d = depth_constants_offcenter_lh(near, far);
	return float4x4(
    float4((2.0f*near) / (right-left),  0.0f,                        0.0f, 0.0f),
    float4(0.0f,                        (2.0f*near) / (top-bottom),  0.0f, 0.0f),
    float4((left+right) / (left-right), (top+bottom) / (bottom-top),  d.x, 1.0f),
    float4(0.0f,                        0.0f,                         d.y, 0.0f));
}

float4x4 proj_offcenter_lh(const float4x4& display_transform, const float3& eye, float near)
{
	// Get the position and dimensions of the output
	float3 display_scale, shear, display_rotation, display_position;
	float4 display_perspective;
	decompose(display_transform, &display_scale, &shear, &display_rotation, &display_position, &display_perspective);
	float3 offset = display_position - eye;

	// Get the basis of the output
	float3 output_basisX = normalize(display_transform[0].xyz());
	float3 output_basisY = normalize(display_transform[1].xyz());
	float3 output_basisZ = normalize(display_transform[2].xyz());

	// Get local offsets from the eye to the output
	float w = dot(output_basisX, offset);
	float h = dot(output_basisY, offset);
	float d = dot(output_basisZ, offset);

	// apply user near plane adjustment
	float zn = max(d + near, 0.01f);
	float depth_scale = zn / d;
	return proj_offcenter_lh(w * depth_scale, (w + display_scale.x) * depth_scale, h * depth_scale, (h + display_scale.y) * depth_scale, zn);
}

frustum_t proj_frustum(const float4x4& proj, bool normalize_planes)
{
	// Gil Gribb & Klaus Hartmann, assumed public domain
	// http://www2.ravensoft.com/users/ggribb/plane%20extraction.pdf
	// http://crazyjoke.free.fr/doc/3D/plane%20extraction.pdf

	frustum_t f;

	f[oCUBE_LEFT].x = proj[0][3] + proj[0][0];
	f[oCUBE_LEFT].y = proj[1][3] + proj[1][0];
	f[oCUBE_LEFT].z = proj[2][3] + proj[2][0];
	f[oCUBE_LEFT].w = proj[3][3] + proj[3][0];

	f[oCUBE_RIGHT].x = proj[0][3] - proj[0][0];
	f[oCUBE_RIGHT].y = proj[1][3] - proj[1][0];
	f[oCUBE_RIGHT].z = proj[2][3] - proj[2][0];
	f[oCUBE_RIGHT].w = proj[3][3] - proj[3][0];

	f[oCUBE_TOP].x = proj[0][3] - proj[0][1];
	f[oCUBE_TOP].y = proj[1][3] - proj[1][1];
	f[oCUBE_TOP].z = proj[2][3] - proj[2][1];
	f[oCUBE_TOP].w = proj[3][3] - proj[3][1];

	f[oCUBE_BOTTOM].x = proj[0][3] + proj[0][1];
	f[oCUBE_BOTTOM].y = proj[1][3] + proj[1][1];
	f[oCUBE_BOTTOM].z = proj[2][3] + proj[2][1];
	f[oCUBE_BOTTOM].w = proj[3][3] + proj[3][1];

	if (has_projection(proj))
	{
		f[oCUBE_NEAR].x = proj[0][2];
		f[oCUBE_NEAR].y = proj[1][2];
		f[oCUBE_NEAR].z = proj[2][2];
		f[oCUBE_NEAR].w = proj[3][2];
	}

	else
	{
		f[oCUBE_NEAR].x = proj[0][3] + proj[0][2];
		f[oCUBE_NEAR].y = proj[1][3] + proj[1][2];
		f[oCUBE_NEAR].z = proj[2][3] + proj[2][2];
		f[oCUBE_NEAR].w = proj[3][3] + proj[3][2];
	}

	f[oCUBE_FAR].x = proj[0][3] - proj[0][2];
	f[oCUBE_FAR].y = proj[1][3] - proj[1][2];
	f[oCUBE_FAR].z = proj[2][3] - proj[2][2];
	f[oCUBE_FAR].w = proj[3][3] - proj[3][2];

  if (normalize_planes)
		for (auto& p : f)
			p = normalize_plane(p);
	
	return f;
}

// returns true if out_corners contains the specified frustum's 8 corner points
// or false of the frustum is poorly formed.
bool proj_corners(const frustum_t& frust, float3 out_corners[8])
{
	bool isect =     plane_v_plane_v_plane(frust[oCUBE_LEFT],  frust[oCUBE_TOP],    frust[oCUBE_NEAR], out_corners + oCUBE_LEFT_TOP_NEAR);
	isect = isect && plane_v_plane_v_plane(frust[oCUBE_LEFT],  frust[oCUBE_TOP],    frust[oCUBE_FAR],  out_corners + oCUBE_LEFT_TOP_FAR);
	isect = isect && plane_v_plane_v_plane(frust[oCUBE_LEFT],  frust[oCUBE_BOTTOM], frust[oCUBE_NEAR], out_corners + oCUBE_LEFT_BOTTOM_NEAR);
	isect = isect && plane_v_plane_v_plane(frust[oCUBE_LEFT],  frust[oCUBE_BOTTOM], frust[oCUBE_FAR],  out_corners + oCUBE_LEFT_BOTTOM_FAR);
	isect = isect && plane_v_plane_v_plane(frust[oCUBE_RIGHT], frust[oCUBE_TOP],    frust[oCUBE_NEAR], out_corners + oCUBE_RIGHT_TOP_NEAR);
	isect = isect && plane_v_plane_v_plane(frust[oCUBE_RIGHT], frust[oCUBE_TOP],    frust[oCUBE_FAR],  out_corners + oCUBE_RIGHT_TOP_FAR);
	isect = isect && plane_v_plane_v_plane(frust[oCUBE_RIGHT], frust[oCUBE_BOTTOM], frust[oCUBE_NEAR], out_corners + oCUBE_RIGHT_BOTTOM_NEAR);
	isect = isect && plane_v_plane_v_plane(frust[oCUBE_RIGHT], frust[oCUBE_BOTTOM], frust[oCUBE_FAR],  out_corners + oCUBE_RIGHT_BOTTOM_FAR);
	return isect;
}

float3 unproject(const float2& screen_position, float VSdepth, const float2& viewport_position, const float2& viewport_dimensions, const float4x4& proj)
{
	float2 screen_texcoord = (screen_position.xy() - viewport_position) / (viewport_dimensions - 1.0f);
	screen_texcoord.y      = 1.0f - screen_texcoord.y;
	float4 clip            = float4(VSdepth * (screen_texcoord * 2.0f - 1.0f), VSdepth * proj[2].z, VSdepth);
	float4 result          = mul(invert(proj), clip);
	return result.xyz();
}

}
