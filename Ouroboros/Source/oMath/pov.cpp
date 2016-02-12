// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/pov.h>
#include <oMath/projection.h>
#include <oMath/matrix.h>

// @todo add focal length
// http://www.bobatkins.com/photography/technical/field_of_view.html


namespace ouro {

pov_t::pov_t(const uint2& screen_dimensions, float fovx, float near, float far)
	: viewport_(screen_dimensions)
	, view_(kIdentity4x4)
{
	projection(fovx, near, far);
}

void pov_t::focus(const float4& sphere)
{
	view(translate_view_to_fit(view_, fov_.y, sphere));
}

void pov_t::projection(float fovx, float near, float far)
{
	fov_.x = fovx;
	near_  = near;
	far_   = far;
	update();
}

void pov_t::viewport(const viewport_t& viewport)
{
	viewport_ = viewport;
	update();
}

float3 pov_t::unproject(const float2& screen_position, float VSdepth) const
{
	return view_to_world(ouro::unproject(screen_position, VSdepth, viewport().position, viewport().dimensions, projection()));
}

float2 pov_t::unprojected_delta(const float2& old_screen_position, const float2& new_screen_position) const
{
	float depth    = pointer_depth_;
	float far_dist = far_;

	// If not set, use an average value
	if (depth <= 0.0f || depth > (far_dist * 0.95f))
		depth = far_dist * 0.15f;

	float2 SSdelta = new_screen_position - old_screen_position;
	float SSdist   = length(SSdelta);

	if (!equal(SSdist, 0.0f))
	{
		const float3 WSold = unproject(old_screen_position, depth);
		const float3 WSnew = unproject(new_screen_position, depth);
		const float WSdist = distance(WSold, WSnew);
		SSdelta *= WSdist / SSdist;
	}

	return SSdelta;
}

//float3 pov_t::reconstruct_position(const float2& screen_position, float linear_depth) const
//{
//	// convert screen position into screen coords (normalized)
//	float2 screen_texcoords = (screen_position.xy() - viewport().position) / viewport().dimensions;
//
//	// lerp the corners to get the vector (as if this was the VS-PS interpolator doing it)
//	float3 far_top              = lerp(corners_[oCUBE_LEFT_TOP_FAR],     corners_[oCUBE_RIGHT_TOP_FAR],     screen_texcoords.x);
//	float3 far_bottom           = lerp(corners_[oCUBE_LEFT_BOTTOM_FAR],  corners_[oCUBE_RIGHT_BOTTOM_FAR],  screen_texcoords.x);
//	float3 far                  = lerp(far_top, far_bottom, screen_texcoords.y);
//
//	float3 near_top             = lerp(corners_[oCUBE_LEFT_TOP_NEAR],    corners_[oCUBE_RIGHT_TOP_NEAR],    screen_texcoords.x);
//	float3 near_bottom          = lerp(corners_[oCUBE_LEFT_BOTTOM_NEAR], corners_[oCUBE_RIGHT_BOTTOM_NEAR], screen_texcoords.x);
//	float3 near                 = lerp(near_top, near_bottom, screen_texcoords.y);
//
//	return lerp(near, far, linear_depth) + position();
//}

void pov_t::update()
{
	//view_                  = always-set-externally;
	view_inverse_            = invert(view_);
	projection_              = proj_fovx_lh(fov_.x, aspect_ratio(), near_dist(), far_dist());
	projection_inverse_      = proj_inv_fovx_lh(fov_.x, aspect_ratio(), near_dist(), far_dist());
	view_projection_         = view_ * projection_;
	view_projection_inverse_ = projection_inverse_ * view_inverse_;
	//fov_.x                 = always-set-externally;
	fov_.y                   = proj_fovy (projection_);
	ratio_                   = proj_ratio(projection_);
	//near_                  = always-set-externally;
	//far_                   = always-set-externally;

	planes_ = proj_frustum(view_projection_);
	proj_corners(planes_, corners_.data());

	const float2 offset = float2(-ratio_.x, ratio_.y) * projection_[2].xy();
	vs_corner_vectors_[oFAR_VEC_TOP_LEFT]     = float3(-ratio_.x + offset.x, -ratio_.y + offset.y, 1.0f);
	vs_corner_vectors_[oFAR_VEC_BOTTOM_LEFT]  = float3(-ratio_.x + offset.x,  ratio_.y + offset.y, 1.0f);
	vs_corner_vectors_[oFAR_VEC_TOP_RIGHT]    = float3( ratio_.x + offset.x, -ratio_.y + offset.y, 1.0f);
	vs_corner_vectors_[oFAR_VEC_BOTTOM_RIGHT] = float3( ratio_.x + offset.x,  ratio_.y + offset.y, 1.0f);
  screen_to_view_scale_bias_ = float4(ratio_.x * 2.0f, ratio_.y * 2.0f, -ratio_.x + offset.x, -ratio_.y + offset.y);

	const float2 half_dimensions = viewport_.dimensions * 0.5f;

	viewport_scale_ = float4(half_dimensions.x, -half_dimensions.y, 1.0f, 1.0f);
	viewport_bias_  = float4(half_dimensions.x,  half_dimensions.y, 0.0f, 0.0f);

	viewport_scale_inverse_ = 1.0f / viewport_scale_;
	viewport_bias_inverse_ = -viewport_bias_ * viewport_scale_inverse_;

	// maps viewport dimensions to [-1,1]
	viewport_to_clip_scale_bias_.x =  2.0f / viewport_.dimensions.x;
	viewport_to_clip_scale_bias_.y = -2.0f / viewport_.dimensions.y;
	viewport_to_clip_scale_bias_.z = -1.0f;
	viewport_to_clip_scale_bias_.w =  1.0f;
}

}
