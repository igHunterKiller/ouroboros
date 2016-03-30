// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oMath/projection.h>
#include <oMath/matrix.h>
#include <oMath/primitive.h>
#include <oMath/subdivide.h>
#include <oMath/texgen.h>
#include <oMesh/primitive.h>
#include <oMesh/mesh.h>
#include <oSurface/surface.h>

using namespace ouro::surface;

namespace ouro { namespace mesh {

struct derive_flags
{
	derive_flags()
		: normals(0)
		, tangents(0)
		, colors(0)
	{}

	uint8_t normals  : 1;
	uint8_t tangents : 1;
	uint8_t colors   : 1;
};

struct source_vertex_data_t
{
	source_vertex_data_t() : indices(nullptr), positions(nullptr), texcoords(nullptr), color(0), num_indices(0), num_vertices(0), type(face_type::unknown) {}

	source_vertex_data_t(const primitive::info_t& info, const primitive::mesh_t& mesh, uint32_t color, const face_type& type)
		: indices(mesh.indices)
		, positions((float3*)mesh.positions)
		, texcoords((float2*)mesh.texcoords)
		, color(color)
		, num_indices(info.nindices)
		, num_vertices(info.nvertices)
		, type(type)
	{}

	const uint16_t* indices;
	const float3*   positions;
	const float2*   texcoords;
	uint32_t        color;
	uint32_t        num_indices;
	uint16_t        num_vertices;
	face_type       type;
};

struct derived_vertex_data_t
{
	derived_vertex_data_t() : normals(nullptr), tangents(nullptr), colors(nullptr) {}
	derived_vertex_data_t(derived_vertex_data_t&& that) : mem(std::move(that.mem)), normals(that.normals), tangents(that.tangents), colors(that.colors)
	{
		that.normals = nullptr; that.tangents = nullptr; that.colors = nullptr;
	}

	blob      mem;
	float3*   normals;
	float4*   tangents;
	uint32_t* colors;

private:
	derived_vertex_data_t(const derived_vertex_data_t&);
	const derived_vertex_data_t& operator=(const derived_vertex_data_t&);
};

static derived_vertex_data_t derive_data(const allocator& alloc, const derive_flags& derive, const source_vertex_data_t& src)
{
	if (!derive.normals && derive.tangents)
		oThrow(std::errc::invalid_argument, "tangents require normals");

	if (!src.texcoords && derive.tangents)
		oThrow(std::errc::invalid_argument, "tangents require texcoords");

	// accumulate memory for the desired derived data
	const size_t normals_bytes  = sizeof(float3)   * src.num_vertices;
	const size_t tangents_bytes = sizeof(float4)   * src.num_vertices;
	const size_t colors_bytes   = sizeof(uint32_t) * src.num_vertices;
	
	size_t req = 0;
	
	if (derive.normals)  req += normals_bytes;
	if (derive.tangents) req += tangents_bytes;
	if (derive.colors)   req += colors_bytes;

	// allocate the combined buffer
	derived_vertex_data_t derived;

	if (req)
	{
		derived.mem = alloc.scoped_allocate(req, "derived vertex data", memory_alignment::align4);
		uint8_t* p  = (uint8_t*)derived.mem;
	
		// assign pointers as data exists
		if (derive.normals)  { derived.normals  = (float3*)  p; p += normals_bytes;	}
		if (derive.tangents) { derived.tangents = (float4*)  p; p += tangents_bytes;	}
		if (derive.colors)   { derived.colors   = (uint32_t*)p; p += colors_bytes;   }

		// calculate data
		if (derived.normals)
			calc_vertex_normals(derived.normals, src.indices, src.num_indices, src.positions, src.num_vertices, true);

		if (derived.tangents)
			calc_vertex_tangents(derived.tangents, src.indices, src.num_indices, src.positions, derived.normals, src.texcoords, src.num_vertices);

		if (derived.colors)
			memset4(derived.colors, src.color, sizeof(uint32_t) * src.num_vertices);
	}

	return derived;
}

static info_t primitive_mesh_info(const face_type& type, const element_t* elements, size_t num_elements, uint32_t num_indices, const float3* positions, uint32_t num_vertices)
{
	info_t info;

	float3 mn, mx;
	calc_aabb(positions, sizeof(float3), num_vertices, &mn, &mx);

	info.num_vertices      = num_vertices;
	info.num_indices       = num_indices;
	info.num_subsets       = 1;
	info.num_slots         = 1;
	info.log2scale         = 0;
	info.primitive_type    = type == face_type::outline ? primitive_type::lines : primitive_type::triangles;
	info.face_type         = type;
	info.flags             = 0;
	info.bounding_sphere   = calc_sphere(positions, sizeof(float3), num_vertices);
	info.extents           = (mx - mn) * 0.5f;
	info.avg_edge_length   = 0.0f;
	info.avg_texel_density = float2(0.0f, 0.0f);
		
	info.layout.fill(celement_t());
	for (size_t i = 0; i < num_elements; i++)
	{
		info.layout[i]      = elements[i];
		info.layout[i].slot = 0; // destination is interleaved
	}
	
	lod_t lod;
	lod.opaque_color.start_subset = 0;
	lod.opaque_color.num_subsets  = 1;
	info.lods.fill(lod);

	return info;
}

static bool has_semantic(const mesh::element_semantic& semantic, const element_t* elements, size_t num_elements)
{
	for (size_t i = 0; i < num_elements; i++)
		if (elements[i].semantic == semantic)
			return true;
	return false;
}

static model primitive_model(const allocator& alloc, const allocator& tmp, const source_vertex_data_t& src, const element_t* dst_elements, size_t dst_num_elements, bool flip_winding_order)
{
	layout_t src_elements;
	uint8_t e                = 0;
	const void* src_verts[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
	src_elements[e]          = celement_t(element_semantic::position, 0, format::r32g32b32_float, e);
	src_verts[e++]           = src.positions;

	if (src.texcoords && has_semantic(element_semantic::texcoord, dst_elements, dst_num_elements))
	{
		src_elements[e] = celement_t(element_semantic::texcoord, 0, format::r32g32_float, e);
		src_verts[e++]  = src.texcoords;
	}

	derive_flags derive;

	if (src.texcoords && has_semantic(element_semantic::normal, dst_elements, dst_num_elements) && has_semantic(element_semantic::tangent, dst_elements, dst_num_elements))
		derive.normals = derive.tangents = true;

	if (has_semantic(element_semantic::color, dst_elements, dst_num_elements))
		derive.colors = true;

	auto derived = derive_data(tmp, derive, src);

	// assign them to source streams
	if (derived.normals)
	{
		src_elements[e] = celement_t(element_semantic::normal, 0, format::r32g32b32_float, e);
		src_verts[e++]  = derived.normals;
	}

	if (derived.tangents)
	{
		src_elements[e] = celement_t(element_semantic::tangent, 0, format::r32g32b32a32_float, e);
		src_verts[e++]  = derived.tangents;
	}

	if (derived.colors)
	{
		src_elements[e] = celement_t(element_semantic::color, 0, format::b8g8r8a8_unorm, e);
		src_verts[e++]  = derived.colors;
	}

	// init a subset
	subset_t subset;
	subset.start_index  = 0;
	subset.num_indices  = src.num_indices;
	subset.start_vertex = 0;
	subset.num_vertices = (uint16_t)src.num_vertices;
	subset.subset_flags = 0;
	subset.material_id  = 0;

	face_type type = src.type;
	if (flip_winding_order)
	{
		if (type == face_type::front_ccw)
			type = face_type::front_cw;
		else if (type == face_type::front_cw)
			type = face_type::front_ccw;
	}

	info_t info = primitive_mesh_info(type, dst_elements, dst_num_elements, src.num_indices, src.positions, src.num_vertices);

	// populate the model
	model m(info, alloc, alloc);
	memcpy(m.subsets(), &subset, sizeof(subset_t));
	memcpy(m.indices(), src.indices, sizeof(uint16_t) * src.num_indices);

	if (type != src.type)
	{
		auto indices = m.indices();
		for (uint32_t i = 0; i < src.num_indices; i += 3)
			std::swap(indices[i+1], indices[i+2]);
	}

	// set up dst in one stream
	void* dst[1];
	dst[0] = m.vertices(0);

	copy_vertices(dst, info.layout, src_verts, src_elements, src.num_vertices);

	return m;
}

model box(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const float3& aabb_min, const float3& aabb_max, uint32_t color)
{
	auto  tess       = type == face_type::outline ? primitive::tessellation_type::lines : primitive::tessellation_type::textured;
	auto  info       = primitive::cube_info(tess);
	void* mem        = alloca(info.total_bytes());
	
	primitive::mesh_t mesh(info, mem);
	primitive::cube_tessellate(&mesh, tess);

	source_vertex_data_t src(info, mesh, color, type);
	return primitive_model(alloc, tmp, src, elements, num_elements, type == face_type::front_ccw);
}

model circle(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const uint16_t facet, float radius, uint32_t color)
{
	auto tess        = type == face_type::outline ? primitive::tessellation_type::lines : primitive::tessellation_type::textured;
	auto info        = primitive::circle_info(tess, facet);
	auto mem         = alloca(info.total_bytes());

	primitive::mesh_t mesh(info, mem);
	primitive::circle_tessellate(&mesh, tess, facet, radius);

	source_vertex_data_t src(info, mesh, color, type);
	return primitive_model(alloc, tmp, src, elements, num_elements, type == face_type::front_ccw);
}

model cylinder(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const uint16_t facet, const uint16_t divide
	, float base_radius, float apex_radius, float height, uint32_t color)
{
	const float r    = apex_radius / std::max(base_radius, oVERY_SMALLf);
	
	auto tess        = type == face_type::outline ? primitive::tessellation_type::lines : primitive::tessellation_type::textured;
	auto info        = primitive::cylinder_info(tess, facet);
	auto mem         = alloca(info.total_bytes());

	primitive::mesh_t mesh(info, mem);
	primitive::cylinder_tessellate(&mesh, tess, facet, r);

	// scale up to size
	if (!equal(base_radius, 1.0f))
	{
		float4x4 scale_tx = scale(base_radius);
		float3* p = (float3*)mesh.positions;
		for (uint32_t i = 0; i < info.nvertices; i++)
			p[i] = mul(scale_tx, p[i]);
	}

	source_vertex_data_t src(info, mesh, color, type);
	return primitive_model(alloc, tmp, src, elements, num_elements, type == face_type::front_ccw);
}

model frustum(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const float4x4& projection, uint32_t color)
{
	auto  tess       = type == face_type::outline ? primitive::tessellation_type::lines : primitive::tessellation_type::textured;
	auto  info       = primitive::cube_info(tess);
	void* mem        = alloca(info.total_bytes());

	primitive::mesh_t mesh(info, mem);
	primitive::cube_tessellate(&mesh, tess);

	proj_corners(projection, (float3*)mesh.positions);

	source_vertex_data_t src(info, mesh, color, type);
	return primitive_model(alloc, tmp, src, elements, num_elements, type == face_type::front_ccw);
}

static void new_sphere_vertex(uint16_t new_index, uint16_t a, uint16_t b, void* user)
{
	auto& mesh = *(primitive::mesh_t*)user;

	mesh.texcoords[new_index * 2    ] = (mesh.texcoords[a * 2    ] + mesh.texcoords[b * 2    ]) * 0.5f;
	mesh.texcoords[new_index * 2 + 1] = (mesh.texcoords[a * 2 + 1] + mesh.texcoords[b * 2 + 1]) * 0.5f;
}

model sphere(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, float radius, uint32_t color)
{
	// outline is the wireframe of a lower-subdivision
	const auto     platonic             = primitive::icosahedron_info(primitive::tessellation_type::solid);
	const uint16_t divide               = type == face_type::outline ? 1 : (platonic.maxdiv - 2);
	      auto     subdivided           = platonic;
				         subdivided.nindices  = calc_max_indices(platonic.nindices,   divide);
	               subdivided.nvertices = calc_max_vertices(platonic.nvertices, divide) * 3 / 2; // extra for texcoord seam fix-up
								 subdivided.type      = primitive::tessellation_type::textured;
	      uint32_t nindices             = platonic.nindices;
	      uint16_t nverts               = platonic.nvertices;
	      auto     mem                  = tmp.scoped_allocate(subdivided.total_bytes(), "sphere subdivision");
	
	primitive::mesh_t mesh(subdivided, mem);

	// install initial platonic and subdivide
	primitive::icosahedron_tessellate(&mesh, platonic.type);
	subdivide(divide, mesh.indices, nindices, mesh.positions, nverts, new_sphere_vertex, &mesh);

	// move positions to surface of sphere & calculate texcoords
	float3* positions = (float3*)mesh.positions;
	float2* texcoords = (float2*)mesh.texcoords;
	for (uint16_t i = 0; i < nverts; i++)
	{
		positions[i] = normalize(positions[i]) * radius;

		if (texcoords)
			primitive::cylindrical_texgen(positions[i].x, positions[i].y, positions[i].z, -radius, radius, &texcoords[i].x, &texcoords[i].y);
	}

	// there are two types of seams left by cylindrical projection texgen
	// 1. Longitudinal: Imagine traveling around the sphere laying down texcoords, 
	//    starting at 0.0. Eventually the last triangle meets the first in this 
	//    traversal, but there a vertex with ~0.9 texcoord u meets the vertex with 
	//    the starting 0.0, thus the texture wraps from 0.9 to zero creating the seam.
	// 2. Polar: A cylindrical projection on a sphere pinches towards the poles where 
	//    there is 1 vertex shared by many triangles radiating in all directions and 
	//    thus a texture coord singularity.

	// Longitudinal fix: find where there's a discontinuity in texcoord u, duplicate
	// the vertex with the lowest u value and increment it by 1.

	// fix seams where 0.9-ish wraps back to 0: duplicate a vertex and set its texcoord to 1
	if (texcoords)
	{
		uint16_t* indices = mesh.indices;

		static const float max_u_delta = 0.75f;
		for (uint32_t i = 0; i < nindices; i += 3)
		{
			primitive::fix_cylindrical_seam(max_u_delta, indices[i+0], indices[i+1], mesh.positions, mesh.texcoords, nverts);
			primitive::fix_cylindrical_seam(max_u_delta, indices[i+0], indices[i+2], mesh.positions, mesh.texcoords, nverts);
			primitive::fix_cylindrical_seam(max_u_delta, indices[i+1], indices[i+2], mesh.positions, mesh.texcoords, nverts);
		}
	}

	// fix the poles
	if (platonic.apex  >= 0) primitive::fix_polar_seam(platonic.apex,  mesh.indices, platonic.nindices, mesh.positions, mesh.texcoords, nverts);
	if (platonic.nadir >= 0) primitive::fix_polar_seam(platonic.nadir, mesh.indices, platonic.nindices, mesh.positions, mesh.texcoords, nverts);

	// check that everything stayed within calculated budgets
	oCheck(nindices <= subdivided.nindices, std::errc::no_buffer_space, "more indices calculated than anticipated (%u > %u max)", nindices, subdivided.nindices);
	oCheck(nverts <= subdivided.nvertices, std::errc::no_buffer_space, "more vertices calculated than anticipated (%u > %u max)", nverts, subdivided.nvertices);

	blob lines;
	if (type == face_type::outline)
	{
		size_t line_bytes = sizeof(uint16_t) * subdivided.nindices * 2;
		lines             = tmp.scoped_allocate(line_bytes, "sphere tessellation lines");
		uint16_t* dst     = (uint16_t*)lines;

		// write lines for each face edge into another buffer
		uint32_t j = 0;
		for (uint32_t i = 0; i < nindices; i += 3)
		{
			dst[j++] = mesh.indices[i+0];
			dst[j++] = mesh.indices[i+1];
			dst[j++] = mesh.indices[i+1];
			dst[j++] = mesh.indices[i+2];
			dst[j++] = mesh.indices[i+2];
			dst[j++] = mesh.indices[i+0];
		}

		nindices = j;
	}

	source_vertex_data_t src;
	src.indices      = type == face_type::outline ? (uint16_t*)lines : mesh.indices;
	src.positions    = positions;
	src.texcoords    = texcoords;
	src.color        = color;
	src.num_indices  = nindices;
	src.num_vertices = nverts;
	src.type         = type;

	return primitive_model(alloc, tmp, src, elements, num_elements, type == face_type::front_ccw);
}

model torus(const allocator& alloc, const allocator& tmp, const face_type& type, const element_t* elements, size_t num_elements
	, const uint16_t facet, const uint16_t divide
	, float inner_radius, float outer_radius, uint32_t color)
{
	float r    = inner_radius / std::max(outer_radius, oVERY_SMALLf);
	auto  tess = type == face_type::outline ? primitive::tessellation_type::lines : primitive::tessellation_type::textured;
	auto  info = primitive::torus_info(tess, facet, divide);
	auto  mem  = alloca(info.total_bytes());

	primitive::mesh_t mesh(info, mem);
	primitive::torus_tessellate(&mesh, tess, facet, divide, r);

	// scale up to size
	if (!equal(outer_radius, 1.0f))
	{
		float4x4 scale_tx = scale(outer_radius);
		float3* p = (float3*)mesh.positions;
		for (uint32_t i = 0; i < info.nvertices; i++)
			p[i] = mul(scale_tx, p[i]);
	}

	source_vertex_data_t src(info, mesh, color, type);
	return primitive_model(alloc, tmp, src, elements, num_elements, type == face_type::front_ccw);
}

}}
