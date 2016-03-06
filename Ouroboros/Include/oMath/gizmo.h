// Copyright (c) 2014 Antony Arciuolo. See License.txt regarding use.

// user interface for transforming an object, similar to max/maya gizmos

#pragma once
#include <oMath/hlsl.h>
#include <oMath/vector_component.h>
#include <cstdint>

namespace ouro {

class gizmo
{
public:
	typedef void (*update_fn)(const float4x4& transform, void* user);

	enum class type : int8_t
	{
		none      = -1,
		scale     =  0,
		rotate    =  1,
		translate =  2,
		count     =  3,
	};

	typedef enum class type type_t;

	enum class space : int8_t
	{
		local,
		world,

		count,
	};

	typedef enum class space space_t;

	enum class state : int8_t
	{
		newly_inactive, // active last frame, inactive or inactive_hover this frame
		inactive,       // no interaction for at least a frame
		inactive_hover, // over the control but not activated
		newly_active,   // inactive or inactive_hover last frame, active this frame
		active,         // active for at least last frame and this frame

		count,
	};

	typedef enum class state state_t;

	// when tessellating the control, it provides this information for a lines list
	// and/or a triangle list. Use opaque per-vertex color rendering.
	struct vertex_t
	{
		float3   position;
		uint32_t color; // abgr
	};

	struct tessellation_info_t
	{
		float4x4         visual_transform;
		float3           eye;
		float            axis_scale;
		float            viewport_scale;
		uint16_t         num_line_vertices;
		uint16_t         num_triangle_vertices;
		type_t           type;
		vector_component selected_axis;
	};


	// === infrequent state setting ===

	// ctor
	gizmo();

	// this function will be called when the transform is updated
	void set_transform_callback(update_fn fn, void* user) { cb_ = fn; cb_user_ = user; }

	// modify the type of gizmo being used
	type type() const { return type_; }
	void type(const type_t& t);

	// modify the space in which manipulation is done
	space space() const { return space_; }
	void space(const space_t& s);

	// 'teleport' the transform to this state, updating the activation tx too, so if abort()
	// is called the manipulation will be restored to this
	void transform(const float4x4& tx);
	float4x4 transform() const;


	// === per-frame logic control ===

	// returns rendering info
	tessellation_info_t update(const float2& viewport_dimensions, const float4x4& inverse_view, const float3& ws_pick0, const float3& ws_pick1, bool activate);

	// returns the result of the last update call, or inactive if abort had been called
	state_t state() const { return state_; }

	// restores activation transform - call if an active manipulation looses input
	// note: pay attention to thread safety because this calls the callback to restore 
	// the transform.
	void reset();

	size_t to_string(char* dst, size_t dst_size) const;


	// === per-frame tessellation ===
	
	// output vertices should be sized to contain update_result's counts for lines & faces
	static void tessellate(const tessellation_info_t& info, vertex_t* out_lines, vertex_t* out_faces);

	void trace_tessellation_info(const tessellation_info_t& info);

private:
	update_fn        cb_;                // callback when the transform is updated
	void*            cb_user_;           // user data for more context to the callback
	float4x4         tx_;                // transform being manipulated
	float4x4         activation_tx_;     // transform as it was when manipulation starts
	float3           activation_offset_; // on active: scale/translation: pick offset from center along axis | rotation: pick on planar axis ring
	float            active_axis_scale_; // used to size a scale manip's axis
	type_t           type_;              // current gizmo experience
	vector_component axis_;              // currently hover-active or selected axis
	space_t          space_;             // use cardinal basis or local basis defined by transform
	state_t          state_;             // state based on pick ray and activation state

	// debug spew of state only when it changes
	void trace();
};

}
