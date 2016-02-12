// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oMath/hlsl.h>
#include <cstdint>

namespace ouro { namespace gfx {

// volume shape: point, box, sphere, cylinder capsule

class pivot_t
{
public:
	enum flag
	{
		tx_mirrored      = 1<<0, // transform is mirrored
		tx_uniform_scale = 1<<1, // scale's xyz are equal
		tx_unscaled      = 1<<2, // scale is uniform and equal to 1
	};

	pivot_t(uint64_t uid, const float4x4& WSposition, const float4& LSsphere, const float3& LSextents);

// _____________________________________________________________________________
// Geometry

	const float4x4& world() const { return world_; }
	const float3 position() const { return world_[3].xyz(); }
	
	const float4&   local_bound()       const { return bound_;   }
	float           local_radius()      const { return bound_.w; }
	float3          local_extents()     const { return extents_; }

	float           world_radius()      const { return bound_.w * world_scale_; }
	float4          world_bound()       const { return float4(position() + bound_.xyz(), world_radius()); }

	uint64_t uid() const { return uid_; }
	
	void obb(float3x3* out_rotation, float3* out_position, float3* out_extents) const;
	void world(const float4x4& world);

	void local(const float3& position, float radius, const float3& extents);

private:

// _____________________________________________________________________________
// Geometry

	float4x4 world_;       // local to world matrix
  float4   bound_;       // local space bounding sphere
  float3   extents_;     // local space extents
  float    world_scale_; // scale from world matrix that can be applied to the local space bounding sphere to get the world-space size

// _____________________________________________________________________________
// Logical
	
	uint64_t uid_;         // unique identifier, persistent across serialization
	uint8_t  generation_;  // increments each time this object is reloaded
	uint8_t  type_;        // type of the pivot
	uint16_t pivot_flags_; // flags associated with the pivot
	uint32_t unusedA_;     // unused
};
static_assert(sizeof(pivot_t) == (7 * sizeof(float4)), "unexpected size");

#if 0 // glimpses of the future

class volume : public pivot
{
	// type
	// flags
	// contents ID or other generic payload
};

class prop : public pivot
{
	uint32_t flags_;
	uint16_t skinning_index_; // index into an array that contains a per-instance copy of a skinned post of the underlying model

	const model* model_;
	const model* swap_to_model_; // swapping to this model once it is safe to do so

	// To add:
	// zbias
	// motionblur overrides, cached values
	// { cloth, physics, vertex paint, any world-space association }
	// material overrides
	// float4x4 skeleton pose
};

class light : public pivot
{
	// type (point, spot, etc.)
	// flags
	// color
	// light params
	// shadow reference
};

#endif

}}
