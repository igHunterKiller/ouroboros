// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/gizmo.h>

#include <oCore/assert.h>
#include <oMemory/xxhash.h>

#include <oCore/color.h>
#include <oCore/stringize.h>

#include <oMath/hlslx.h>
#include <oMath/matrix.h>
#include <oMath/primitive.h>
#include <oMath/quat.h>
#include <oMath/seg_closest_points.h>
#include <oMath/seg_v_plane.h>
#include <oMath/seg_v_seg.h>
#include <oMath/seg_v_sphere.h>
#include <oMath/seg_v_torus.h>

namespace ouro {

template<> const char* as_string(const gizmo::type_t& t)
{
	static const char* s_names[] = { "none", "scale", "rotate", "translate" };
	match_array(s_names, gizmo::type::count + 1);
	return s_names[(int)t+1];
}

template<> const char* as_string(const gizmo::space_t& s)
{
	static const char* s_names[] = { "local", "world", };
	return as_string(s, s_names);
}

template<> const char* as_string(const gizmo::state_t& s)
{
	static const char* s_names[] = { "newly_inactive", "inactive", "inactive_hover", "newly_active", "active" };
	return as_string(s, s_names);
}

namespace vector_component
{
enum value : int8_t {

	none = -1,
	x = 0,
	y = 1,
	z = 2,
	w = 3,
	count = 4,

};
}

// define constants for scaling, color, etc.
static const uint32_t s_selected_color = color::white;
static const uint32_t s_ghost_color = color::dark_slate_gray;
static const uint32_t s_component_colors[vector_component::count] = { color::red, color::green, color::blue, color::yellow };
static const uint16_t s_facet = 32;
static const float    s_select_eps = 0.01f;
static const float    s_axis_length = 0.2f;
static const float    s_ss_translation_ring_radius = 0.02f;
static const float    s_ss_rotation_ring_radius = 0.25f;
static const float    s_cap_scale = 0.0115f;
static const float    s_cube_radius = 0.017321f;

static const uint16_t s_num_lines[(int)gizmo::type::count + 1] = { 0, 3, 5 * s_facet, 3 + s_facet };
static const uint16_t s_num_tris[(int)gizmo::type::count + 1] = { 0, 4 * 12, 0, 3 * 2 * (s_facet - 1) };

vector_component::value initial_pick(
	const gizmo::type_t& type // scale, rotation, translation
	, const float4x4& transform           // manipulator transform
	, const float3& eye                   // camera position
	, float selection_epsilon             // allow for more forgiving selection
	, const float& axis_radius            // ring radius or axis length
	, const float3& ws_pick0              // one point of a pick line segment
	, const float3& ws_pick1              // the other point of a pick line segment
	, float3* out_offset)                 // receives the offset from the center of the click
{
	// dist is used to scale values so they seem fixed in screen-space
	float t0, t1;
	const float    fixed_ss_rotate_scale = 1.25f;
	const float3   center = transform[3].xyz();
	const float    dist = length(eye - center);
	const float3   eye_dir = (eye - center) / dist;
	const float4   screen_plane = plane(eye_dir, center);
	const float4x4 fixed_tx = rescale(transform, dist);
	const float    fixed_selection_epsilon = dist * selection_epsilon;
	const float    fixed_radius = dist * axis_radius;
	const float    fixed_ss_translation_ring_radius = dist * s_ss_translation_ring_radius;
	const float    fixed_cap_radius = dist * (s_cube_radius + selection_epsilon);

	// assume no intersection
	auto picked = vector_component::none;
	float3 pt;

	switch (type)
	{
		case gizmo::type::scale:
		{
			// interaction elements: endpoints of axes for scale in each cardinal direction
			// and one at the center for uniform scaling.

			for (int i = 0; i < vector_component::count; i++)
			{
				auto component = (vector_component::value)i;

				// 'fixed' gets applied as part of fixed_tx, so use original radii here
				float3 end = kZero3;
				if (component != vector_component::w)
					end[i] = axis_radius;

				// create a bounds for the end point
				const float4 sphere(mul(fixed_tx, end), fixed_cap_radius);

				// intersect 
				if (seg_v_sphere(ws_pick0, ws_pick1, sphere, &t0, &t1))
				{
					if (component == vector_component::w)
					{
						// uniform scale: offset is distance from sphere intersection to center

						const float3 point = ws_pick0 + t0 * (ws_pick1 - ws_pick0);
						*out_offset = point - center;
					}

					else
					{
						// axis scale: offset is distance from center to the pick point projected onto the axis

						const float3 p1 = sphere.xyz();
						seg_closest_points(center, p1, ws_pick0, ws_pick1, &t0, &t1);
						*out_offset = lerp(center, p1, t0) - center;
					}

					picked = component;
				}
			}

			break;
		}

		case gizmo::type::rotate:
		{
			// interaction elements: 3 axis rings plus a screen-space rotation ring on the outside

			float4 ring_plane;

			// check each rotation axis
			for (int i = 0; i < vector_component::w; i++)
			{
				float3 axis = kZero3;
				axis[i] = 1.0f;
				axis = mul((float3x3)transform, axis);

				// intersect only the part of the ring on the front side of the manipulator
				if (seg_v_torus(ws_pick0, ws_pick1, center, axis, fixed_radius, fixed_selection_epsilon, &pt) && in_front_of(screen_plane, pt))
				{
					picked = (vector_component::value)i;
					ring_plane = plane(axis, center);
					break;
				}
			}

			// if no axis was selected, check the screen-space ring
			if (picked == vector_component::none && seg_v_torus(ws_pick0, ws_pick1, center, screen_plane.xyz(), fixed_radius * fixed_ss_rotate_scale, fixed_selection_epsilon, &pt))
			{
				picked = vector_component::w;
				ring_plane = screen_plane;
			}

			// project onto the plane of the ring/torus to remove the epsilon of the minor radius
			// then make this an offset from the center point like other offsets
			*out_offset = project(ring_plane, pt) - center;

			break;
		}

		case gizmo::type::translate:
		{
			// interaction elements: 3 axes + a screen-space translation handle at the center

			const float4 sphere = float4(center, fixed_ss_translation_ring_radius);

			// check screen-space translation circle first
			if (seg_v_sphere(ws_pick0, ws_pick1, sphere, &t0, &t1))
			{
				seg_v_plane(ws_pick0, ws_pick1, screen_plane, &pt);
				*out_offset = center - pt;
				picked = vector_component::w;
			}

			else
			{
				// check each axis
				for (int i = 0; i < vector_component::w; i++)
				{
					float3 end = kZero3;
					end[i] = fixed_radius;
					end = mul(fixed_tx, end);

					if (seg_vs_seg(center, end, ws_pick0, ws_pick1, fixed_selection_epsilon, &t0, &t1, &pt))
					{
						seg_closest_points(center, end, ws_pick0, ws_pick1, &t0, &t1);
						pt = lerp(center, end, t0);
						*out_offset = center - pt;
						picked = (vector_component::value)i;
						break;
					}
				}
			}

			break;
		}

		default:
			break;
	}

	return picked;
}

static void tess_axes(const float4x4& transform, float extent, float selected_extent, const vector_component::value& selected_axis, const uint32_t axis_color[vector_component::count], gizmo::vertex_t*& out_vertices)
{
	const float3 center = transform[3].xyz();
	for (int i = 0; i < vector_component::w; i++)
	{
		auto component = (vector_component::value)i;

		float3 pt = kZero3;
		pt[i] = component == selected_axis ? selected_extent : extent;

		out_vertices->position = center;
		out_vertices->color = (uint32_t)axis_color[i];
		out_vertices++;
		out_vertices->position = mul(transform, pt);
		out_vertices->color = (uint32_t)axis_color[i];
		out_vertices++;
	}
}

static void tess_ring(const float4x4& transform, uint32_t color, const float4& screen_plane, uint16_t facet, const float3* ring_verts_xy, gizmo::vertex_t*& out_vertices)
{
	const bool front_only = any(screen_plane != 0.0f);

	float3 v1, v0 = mul(transform, ring_verts_xy[facet-1]);
	bool   f1, f0 = !front_only || in_front_of(screen_plane, v0);

	int nskipped = 0;
	for (uint16_t i = 0; i < facet; i++, v0 = v1, f0 = f1)
	{
		v1 = mul(transform, ring_verts_xy[i]);
		f1 = !front_only || in_front_of(screen_plane, v1);

		// if both line verts are behind, skip
		if (!f0 && !f1)
		{
			nskipped += 2;
			continue;
		}

		// if line crosses, clip to plane
		if (f0 != f1)
		{
			float3 pt;
			seg_v_plane(v0, v1, screen_plane, &pt);
			if (f0) v1 = pt;
			else    v0 = pt;
		}

		// else both line verts are in front, draw normally

		out_vertices->position = v0;
		out_vertices->color = color;
		out_vertices++;

		out_vertices->position = v1;
		out_vertices->color = color;
		out_vertices++;
	}

	// pad out to keep draws always to the same number of verts
	for (int i = 0; i < nskipped; i++)
	{
		out_vertices->position = v0;
		out_vertices->color = color;
		out_vertices++;
	}
}

gizmo::gizmo()
	: cb_(nullptr)
	, cb_user_(nullptr)
	, tx_(kIdentity4x4)
	, activation_tx_(kIdentity4x4)
	, active_axis_scale_(1.0f)
	, activation_offset_(kZero3)
	, type_(gizmo::type::none)
	, axis_(vector_component::none)
	, space_(space::world)
	, state_(state::inactive)
{
}

void gizmo::type(const type_t& t)
{
	axis_ = vector_component::none;
	state_ = state::newly_inactive;
	type_ = t;
}

void gizmo::space(const space_t& s)
{
	axis_ = vector_component::none;
	state_ = state::newly_inactive;
	space_ = s;
}

void gizmo::reset()
{
	axis_ = vector_component::none;

	if (state_ >= state::newly_active)
	{
		tx_ = activation_tx_;
		state_ = state::newly_inactive;

		if (cb_)
			cb_(tx_, cb_user_);
	}
}

void gizmo::transform(const float4x4& tx)
{
	activation_tx_ = tx;
	tx_ = tx;
}

float4x4 gizmo::transform() const
{
	return tx_;
}

gizmo::tessellation_info_t gizmo::update(const float2& viewport_dimensions, const float4x4& inverse_view, const float3& ws_pick0, const float3& ws_pick1, bool activate)
{
	// override activation if the type is none
	if (type_ == gizmo::type::none)
		activate = false;

	// extract initial parameters
	const float    vp_scale = 1.0f / max(viewport_dimensions);
	const float3   eye      = inverse_view[3].xyz();
	const float3   right    = inverse_view[0].xyz();
	const float4x4 pick_tx  = remove_shear(space_ == space::world ? translate(tx_[3].xyz()) : tx_);
	const float3   center   = pick_tx[3].xyz();
	float dist;
	const float3 eye_dir    = normalize(eye - center, dist);

	// run initial pick whenever not active to handle hover and initial selection
	// detect state change
	const bool was_active = state_ >= state::newly_active;

	if (!was_active)
		axis_ = initial_pick(type_, pick_tx, eye, s_select_eps, s_axis_length, ws_pick0, ws_pick1, &activation_offset_);

	const bool first_active = !was_active &&  activate && axis_ != vector_component::none;
	const bool first_inactive = was_active && !activate;
	const bool has_focus = axis_ != vector_component::none;
	const bool active = activate && axis_ != vector_component::none;

	// update cached state
	if (first_inactive)      state_ = state::newly_inactive;
	else if (first_active)   state_ = state::newly_active;
	else if (active)         state_ = state::active;
	else if (has_focus)      state_ = state::inactive_hover;
	else                     state_ = state::inactive;

	// cache transform in case something goes wrong mid-manipulation (loss of focus/mouse capture)
	if (first_active && has_focus)
		activation_tx_ = tx_;

	// if not manipulating then short-circuit
	if (!activate || !has_focus)
	{
		// Restore length of scale axis when not active
		active_axis_scale_ = 1.0f;
	}

	else
	{
		// perform transform update

		float3 axis_vector;
		if (axis_ == vector_component::w)
			axis_vector = eye_dir;
		else
		{
			axis_vector = kZero3;
			axis_vector[axis_] = 1.0f;
		}

		// take action specific to each manipulator type
		switch (type_)
		{
			case gizmo::type::scale:
			{
				// in SS create a manipulator vector from the activation (down) point 
				// and horizontally to the left; else start from the true center to 
				// the activation point
				const float3 down_pt = center + activation_offset_;
				const float3 ss_offset = right * -s_axis_length * dist;
				const float3 start = axis_ == vector_component::w ? (down_pt + ss_offset) : center;
				const float3 end = down_pt;

				// find where along that axis the new pick is
				float t0, t1;
				seg_closest_points(start, end, ws_pick0, ws_pick1, &t0, &t1);

				// clamp to prevent negative scale
				t0 = max(t0, 0.001f);

				// cache so the axis can be drawn stretched
				active_axis_scale_ = t0;

				// apply the transform
				if (axis_ == vector_component::w)
					tx_ = scale(t0) * activation_tx_;
				else if (space_ == space::local)
					tx_[(int)axis_] = activation_tx_[(int)axis_] * t0;
				else
				{
					float3 sc(1.0f, 1.0f, 1.0f);
					sc[(int)axis_] = t0;

					tx_ = activation_tx_ * scale(sc);
					tx_[3] = activation_tx_[3];
				}

				break;
			}

			case gizmo::type::rotate:
			{
				const float3 normal = (axis_ == vector_component::w) ? eye_dir : mul((float3x3)pick_tx, axis_vector);

				const float4 axis_plane = plane(normal, center);

				// Find the current point on the same ring as was selected at initial pick time
				float3 pt;
				if (seg_v_plane(ws_pick0, ws_pick1, axis_plane, &pt))
				{
					// Because the ring's pick eps is a radius around the ring and not truly on the ring
					// plane itself, there is enough deviation to create an initial rotation jump. Protect 
					// against that on first activation by ensuring delta is zero.
					if (first_active)
						activation_offset_ = pt;
					else
					{
						// Find the angle between the activation point and the current point
						float3 ac = normalize(activation_offset_ - center);
						float3 bc = normalize(pt                 - center);

						tx_ = activation_tx_ * (float4x4)qrotation(qrotate(bc, ac));
						tx_[3] = float4(center, 1.0f);
					}
				}

				break;
			}

			case gizmo::type::translate:
			{
				float3 pt;

				if (axis_ == vector_component::w)
				{
					const float4 axis_plane = plane(axis_vector, center);
					seg_v_plane(ws_pick0, ws_pick1, axis_plane, &pt);
				}

				else
				{
					const float3 end = mul(pick_tx, axis_vector);

					float t0, t1;
					seg_closest_points(center, end, ws_pick0, ws_pick1, &t0, &t1);
					pt = lerp(center, end, t0);
				}

				tx_[3] = float4(pt + activation_offset_, tx_[3].w);

				break;
			}
		}

		if (cb_)
			cb_(tx_, cb_user_);
	}

	// fix up visual transform

	float4x4 visual_tx;
	if (type_ == type::rotate && space_ == space::world && has_focus)
	{
		if (activate)
		{
			visual_tx = invert(remove_scale_shear(activation_tx_)) * rescale(remove_shear(tx_), dist);
			visual_tx[3] = tx_[3];
		}

		else
			visual_tx = translate(tx_[3].xyz());
	}

	else
	{
		visual_tx = rescale(remove_shear(tx_), dist);
		if (space_ == space::world)
			visual_tx = remove_rotation(visual_tx);
	}

	tessellation_info_t info;
	info.visual_transform = visual_tx;
	info.eye = eye;
	info.axis_scale = active_axis_scale_;
	info.num_line_vertices = 2 * s_num_lines[(int)type_ + 1];
	info.num_triangle_vertices = 3 * s_num_tris[(int)type_ + 1];
	info.type = type_;
	info.selected_axis = axis_;
	return info;
}

void gizmo::tessellate(const tessellation_info_t& info, vertex_t* out_lines, vertex_t* out_faces)
{
	// cache information about the focal point
	const auto current_axis = (vector_component::value)info.selected_axis;
	const float scaled_axis_length = s_axis_length * info.axis_scale;
	const float3 eye = info.eye;
	const float4x4 visual_tx = info.visual_transform;
	const float3 center = visual_tx[3].xyz();
	float3 eye_direction = eye - center;
	const float dist = length(eye_direction);
	eye_direction /= dist; // normalize

	// initialize colors based on the axis selected
	uint32_t colors[vector_component::count];
	memcpy(colors, s_component_colors, sizeof(colors));

	if (current_axis != vector_component::none)
		colors[current_axis] = s_selected_color;

	// Prepare ring template
	float3* ring_verts = nullptr;
	{
		auto inf = primitive::circle_info(primitive::tessellation_type::lines, s_facet);
		auto mem = alloca(inf.total_bytes());
		primitive::mesh_t indexed(inf, mem);
		primitive::circle_tessellate(&indexed, inf.type, s_facet, 1.0f);
		ring_verts = (float3*)indexed.positions;
	}

	vertex_t* outl = out_lines;
	vertex_t* outt = out_faces;

	switch (info.type)
	{
		case gizmo::type::scale:
		{
			const float    axis_length = current_axis == vector_component::w ? scaled_axis_length : s_axis_length;
			const float4x4 cap_scale = scale(s_cap_scale);

			tess_axes(visual_tx, axis_length, scaled_axis_length, current_axis, s_component_colors, (vertex_t*)outl);

			for (int i = 0; i < vector_component::count; i++)
			{
				auto component = (vector_component::value)i;

				float3 trans = kZero3;
				if (component != vector_component::w)
					trans[i] = component == current_axis ? scaled_axis_length : axis_length;

				float4x4 tx = cap_scale * translate(trans) * visual_tx;

				{
					auto inf = primitive::cube_info(primitive::tessellation_type::solid);
					auto mem = alloca(inf.total_bytes());

					primitive::mesh_t indexed(inf, mem);
					primitive::cube_tessellate(&indexed, inf.type);

					// deindex into output
					const uint32_t  color = colors[i];
					const uint16_t  nindices = inf.nindices;
					const uint16_t* indices = indexed.indices;
					const float3*   positions = (float3*)indexed.positions;

					for (uint16_t j = 0; j < nindices; j++, out_faces++)
					{
						out_faces->position = mul(tx, positions[indices[j]]);
						out_faces->color = color;
					}
				}
			}

			break;
		}

		case gizmo::type::rotate:
		{
			float4         screen_plane = plane(eye_direction, center);
			const float    fixed_axis_length = dist * s_axis_length;
			const float    fixed_ss_rotation_ring_radius = dist * s_ss_rotation_ring_radius;
			const float4x4 fixed_scale = scale(fixed_axis_length);
			const float4x4 screen_plane_tx = rotate_xy_planar(screen_plane);

			float4x4 space_rotation = remove_scale(visual_tx);
			space_rotation[3] = kWAxis4;

			// use 'none' to indicate the gray ring that completes axis rings that are close to screen-space aligned
			for (int i = vector_component::none; i < vector_component::count; i++)
			{
				auto component = (vector_component::value)i;

				float4 clip_plane = (component == current_axis) ? kZero4 : screen_plane;
				float3 trans = center;

				float4x4 tx;
				switch (component)
				{
					case vector_component::x:    tx = fixed_scale * rotate(float3(0.0f, oPIf * 0.5f, 0.0f)) * space_rotation; break;
					case vector_component::y:    tx = fixed_scale * rotate(float3(oPIf * 0.5f, 0.0f, 0.0f)) * space_rotation; break;
					case vector_component::z:    tx = fixed_scale                                                  * space_rotation; break;
					case vector_component::w:    tx = scale(fixed_ss_rotation_ring_radius) * screen_plane_tx; clip_plane = kZero4;   break;
					case vector_component::none: tx = scale(fixed_axis_length * 1.03f)     * screen_plane_tx; clip_plane = kZero4; trans = trans + -0.1f * eye_direction; break;
				}

				tx[3] = float4(trans, 1.0f);

				tess_ring(tx, component == vector_component::none ? s_ghost_color : colors[i], clip_plane, s_facet, ring_verts, out_lines);
			}

			break;
		}

		case gizmo::type::translate:
		{
			static const float3 s_cap_rotation[vector_component::w] =
			{
				float3(0.0f,         oPIf * 0.5f, 0.0f),
				float3(-oPIf * 0.5f,        0.0f, 0.0f),
				float3(0.0f,                0.0f, 0.0f),
			};

			// 3 axes + center ring
			tess_axes(visual_tx, s_axis_length, s_axis_length, current_axis, colors, outl);

			// dist's contribution to scale is handled in the use of visual_tx
			const float4x4 visual_ss_transform = scale(s_ss_translation_ring_radius)
				* rotate_xy_planar(float4(eye_direction, 0.0f))
				* remove_rotation(visual_tx);

			tess_ring(visual_ss_transform, colors[vector_component::w], kZero4, s_facet, ring_verts, outl);

			// dist's contribution to scale is handled in the use of visual_tx
			const float scaled_extent = scaled_axis_length;

			uint16_t cone_nverts = 0;
			float3* cone_verts = nullptr;
			{
				auto inf = primitive::cone_info(primitive::tessellation_type::solid, s_facet);
				auto mem = alloca(inf.total_bytes());

				primitive::mesh_t indexed(inf, mem);
				primitive::mesh_t unindexed;

				cone_nverts = inf.nindices;
				unindexed.positions = (float*)alloca(sizeof(float3) * cone_nverts);

				primitive::cone_tessellate(&indexed, inf.type, s_facet);
				primitive::deindex(&unindexed, indexed, inf);

				cone_verts = (float3*)unindexed.positions;
			}

			for (int i = 0; i < vector_component::w; i++)
			{
				auto component = (vector_component::value)i;

				float3 trans = kZero3;
				trans[i] = scaled_extent;

				float4x4 tx = scale(s_cap_scale) * rotate(s_cap_rotation[i]) * translate(trans) * visual_tx;

				for (uint16_t v = 0; v < cone_nverts; v++, outt++)
				{
					outt->position = mul(tx, cone_verts[v]);
					outt->color = colors[i];
				}
			}

			break;
		}

		default:
			break;
	}
}


// @tony: given a list of strings and a list of types, auto-print a struct/class/memory footprint.
// it would have to respect class/struct packing, or assume type list must be explicit. sss.h has 
// a spec, but needs to differentiate int-like tuples from float tupples.
#define oFLOAT2F "%.03f %.03f"
#define oFLOAT3F oFLOAT2F " %.03f"
#define oFLOAT4F oFLOAT3F " %.03f"
#define oFLOAT4x4F_ML(prefix) oFLOAT4F prefix oFLOAT4F prefix oFLOAT4F prefix oFLOAT4F
#define oFLOAT2_PARAMS(f2) (f2).x, (f2).y
#define oFLOAT3_PARAMS(f3) oFLOAT2_PARAMS(f3), (f3).z
#define oFLOAT4_PARAMS(f4) oFLOAT3_PARAMS(f4), (f4).w
#define oFLOAT4x4_AS_PARAMS(f4x4) oFLOAT4_PARAMS((f4x4).col0), oFLOAT4_PARAMS((f4x4).col1), oFLOAT4_PARAMS((f4x4).col2), oFLOAT4_PARAMS((f4x4).col3)

void gizmo::trace()
{
	static uint32_t s_ctr = 0;
	static uint64_t s_hash = 0;
	auto hash = xxhash64(this, sizeof(*this));

	if (s_hash != hash)
	{
		s_hash = hash;

		// could be XXmmvfiiii... pass an array of labels (field names), type codes, to-string functions?
		// maybe make the field codes a part of typeid_

		oTrace("gizmo %p %08u", this, s_ctr++);
		oTrace("{");
		oTrace("\tcb=%p", cb_);
		oTrace("\tcb_user=%p", cb_user_);
		oTrace("\ttx=           " oFLOAT4x4F_ML("\n\t              "), oFLOAT4x4_AS_PARAMS(tx_));
		oTrace("\tactivation_tx=" oFLOAT4x4F_ML("\n\t              "), oFLOAT4x4_AS_PARAMS(activation_tx_));
		oTrace("\tactivation_offset=%.03f %.03f %.03f", oFLOAT3_PARAMS(activation_offset_));
		oTrace("\tactive_axis_scale=%f", active_axis_scale_);
		oTrace("\taxis=%d", axis_);
		oTrace("\ttype=%s", as_string(type_));
		oTrace("\tspace=%s", as_string(space_));
		oTrace("\tstate=%s", as_string(state_));
		oTrace("}");
	}
}

char* to_string(char* dst, size_t dst_size, const gizmo::tessellation_info_t& info)
{
	int len = snprintf(dst, dst_size,
		"gizmo::trace_tessellation_info_t %p\n"                         \
		"{\n"                                                           \
		"\tvisual_transform=" oFLOAT4x4F_ML("\t                 ") "\n" \
		"\teye=" oFLOAT3F "\n"                                          \
		"\taxis_scale=%.03f\n"                                          \
		"\tnum_line_vertices=%u\n"                                      \
		"\tnum_triangle_vertices=%u\n"                                  \
		"\ttype=%s\n"                                                   \
		"\tselected_axis=%d\n"                                          \
		"}\n"
			, &info, oFLOAT4x4_AS_PARAMS(info.visual_transform), oFLOAT3_PARAMS(info.eye), info.axis_scale
			, info.num_line_vertices, info.num_triangle_vertices, as_string(info.type), info.selected_axis);

	return len < dst_size ? dst : nullptr;
}

void gizmo::trace_tessellation_info(const tessellation_info_t& info)
{
	static uint32_t s_ctr = 0;
	static uint64_t s_hash = 0;
	auto hash = xxhash64(this, sizeof(*this));

	if (s_hash != hash)
	{
		s_hash = hash;

		char buf[1024];
		oTrace("%s", to_string(buf, countof(buf), info));
	}
}

}
