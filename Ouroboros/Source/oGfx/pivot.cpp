// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/pivot.h>
#include <oMath/hlslx.h>
#include <oMath/matrix.h>

namespace ouro { namespace gfx {

pivot_t::pivot_t(uint64_t uid, const float4x4& WSposition, const float4& LSsphere, const float3& LSextents)
	: bound_(LSsphere) // @tony should radius be max(extents) or to the vertex? think about a cylinder, capsule too
	, extents_(LSextents)
	, world_scale_(1.0f)
	, uid_(uid)
	, generation_(0)
	, type_(0)
	, pivot_flags_(tx_uniform_scale | tx_unscaled)
	, unusedA_(0)
{
	world(WSposition);
}

void pivot_t::obb(float3x3* out_rotation, float3* out_position, float3* out_extents) const
{
	const float3 s = scale(world_);

	(*out_rotation)[0] = world_[0].xyz() / s.x;
	(*out_rotation)[1] = world_[1].xyz() / s.y;
	(*out_rotation)[2] = world_[2].xyz() / s.z;
	*out_position = mul(world_, bound_.xyz());
	*out_extents  = extents_ * s;
}

void pivot_t::world(const float4x4& world)
{
	world_ = world;
	if (!affine(world))
		throw std::invalid_argument("not affine");

	if (determinant(world) < 0.0f)
		pivot_flags_ |=  tx_mirrored;
	else
		pivot_flags_ &=~ tx_mirrored;

	float3 sc = scale(world_);

	if (equal(sc.x, sc.y) && equal(sc.x, sc.z))
		pivot_flags_ |=  tx_uniform_scale;
	else
		pivot_flags_ &=~ tx_uniform_scale;

	if (equal(sc.x, 1.0f))
		pivot_flags_ |=  tx_unscaled;
	else
		pivot_flags_ &=~ tx_unscaled;

	world_scale_ = max(sc);
}

void pivot_t::local(const float3& position, float radius, const float3& extents)
{
	bound_ = float4(position, radius);
	extents_ = extents;
}

}}
