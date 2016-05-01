// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/model_registry.h>
#include <oMath/matrix.h>
#include <oMesh/codec.h>
#include <oMesh/primitive.h>
#include <oCore/color.h>
#include <oCore/stringize.h>

namespace ouro { 

template<> const char* as_string(const gfx::primitive_model& m)
{
	static const char* s_names[] =
	{
		"circle",
		"pyramid",
		"box",
		"cone",
		"cylinder",
		"sphere",
		"rectangle",
		"torus",
		"circle_outline",
		"pyramid_outline",
		"box_outline",
		"cone_outline",
		"cylinder_outline",
		"sphere_outline",
		"rectangle_outline",
		"torus_outline",
	};
	return as_string(m, s_names);
}
	
namespace gfx {

void model_registry::initialize(gpu::device* dev, uint32_t budget_bytes, const allocator& alloc, const allocator& io_alloc)
{
	auto vlayout = vertex_layout();

	// if there's a static primitives registry, this might be able to turn into an initialize_with_builtin
	mesh::model placeholder = mesh::box(io_alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), float3(-1.0f), float3(1.0f), (uint32_t)color::default_gray);
	auto error_placeholder  = mesh::encode(placeholder, mesh::file_format::omdl, io_alloc, io_alloc);

	alloc_ = alloc ? alloc : default_allocator;

	allocate_options opts(required_alignment);
	void* memory = alloc_.allocate(budget_bytes, "model_registry", opts);

	base_t::initialize("model registry", memory, budget_bytes, dev, error_placeholder, io_alloc);

	insert_primitives(alloc, io_alloc);
	flush();
}

void model_registry::deinitialize()
{
	if (!valid())
		return;

	base_t::deinitialize();
}

void* model_registry::create_resource(const uri_t& uri_ref, blob& compiled)
{
	// todo: find more meaningful allocators here
	const allocator& subsets_allocator = default_allocator;
	const allocator& temp_allocator = default_allocator;

	auto fformat = mesh::get_file_format(compiled);
	oCheck(fformat != mesh::file_format::unknown, std::errc::invalid_argument, "Unknown file format: %s", uri_ref.c_str());

	auto mdl = pool_.create();

	auto vlayout = vertex_layout();

	// todo: figure out a way that this can allocate right out of the cpu-accessible buffer... or hope for D3D12
	*mdl = mesh::decode(uri_ref.path(), compiled, vlayout, subsets_allocator, temp_allocator, temp_allocator);
	auto info = mdl->info();

	gpu::ibv indices = dev_->new_ibv(uri_ref, info.num_indices, mdl->indices());

	std::array<uint32_t, mesh::max_num_slots> vertices_offsets;
	vertices_offsets.fill(0);

	const uint32_t nslots = info.num_slots;
	for (uint32_t slot = 0; slot < nslots; slot++)
	{
		const auto stride = mdl->vertex_stride(slot);
		gpu::vbv verts = dev_->new_vbv(uri_ref, stride, info.num_vertices, mdl->vertices(slot));
		vertices_offsets[slot] = verts.offset;
	}

	mdl->rebase(dev_->readable_mesh_base(), noop_deallocate, indices.offset, vertices_offsets);

	return mdl;
}

void model_registry::destroy_resource(void* entry)
{
	auto mdl = (mesh::model*)entry;
	auto info = mdl->info();

	gpu::ibv indices;
	indices.offset = mdl->indices_offset();
	indices.num_indices = info.num_indices;
	indices.transient = 0;
	indices.is_32bit = false;
	dev_->del_ibv(indices);
	
	const uint32_t nslots = info.num_slots;
	for (uint32_t slot = 0; slot < nslots; slot++)
	{
		gpu::vbv verts;
		verts.offset = mdl->vertices_offset(slot);
		verts.num_vertices = info.num_vertices;
		verts.transient = 0;
		verts.vertex_stride_uints_minus_1 = 0;
		dev_->del_vbv(verts);
	}

  pool_.destroy(mdl);
}

void model_registry::insert_primitive(const primitive_model& prim, const mesh::model& model, const allocator& alloc, const allocator& io_alloc)
{
	auto omdl = mesh::encode(model, mesh::file_format::omdl, alloc, io_alloc);
	insert_indexed((key_type)prim + 1, as_string(prim), omdl);
}

void model_registry::insert_primitives(const allocator& alloc, const allocator& io_alloc)
{
	auto vlayout = vertex_layout();
	const auto& outline_layout = mesh::basic::pos_col;
	static const uint16_t kFacet = 20;
	static const float kExtent = 1.0f;

	const auto rot = radians(45.0f);
	const auto sca = 1.0f / cos(rot);

	// squares are lowly-tessellated circles/cylinders, but the 4 vertices are on the circumference
	// of the sphere, not in the min/max corners, so rotate into position.
	const auto quad_tx = scale(float3(sca, sca, 1.0f)) * rotate(float3(0.0f, 0.0f, rot));

	auto mesh = mesh::box(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), float3(-kExtent), float3(kExtent));
	insert_primitive(primitive_model::box, mesh, alloc, io_alloc);

	mesh = mesh::circle(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), kFacet, kExtent);
	insert_primitive(primitive_model::circle, mesh, alloc, io_alloc);

	mesh = mesh::cylinder(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), kFacet, 1, kExtent, 0.0f, 1.0f);
	insert_primitive(primitive_model::cone, mesh, alloc, io_alloc);

	mesh = mesh::cylinder(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), kFacet, 1, kExtent, kExtent, 2.0f);
	insert_primitive(primitive_model::cylinder, mesh, alloc, io_alloc);

	mesh = mesh::cylinder(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), 4, 1, kExtent, 0.0f, 1.0f);
	mesh.bake_transform(quad_tx);
	insert_primitive(primitive_model::pyramid, mesh, alloc, io_alloc);

	mesh = mesh::circle(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), 4, kExtent);
	mesh.bake_transform(quad_tx);
	insert_primitive(primitive_model::rectangle, mesh, alloc, io_alloc);

	mesh = mesh::cylinder(alloc, io_alloc, mesh::face_type::outline, outline_layout, 4, 1, kExtent, 0.0f, 1.0f);
	mesh.bake_transform(quad_tx);
	insert_primitive(primitive_model::pyramid_outline, mesh, alloc, io_alloc);

	mesh = mesh::sphere(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), kExtent);
	insert_primitive(primitive_model::sphere, mesh, alloc, io_alloc);

	mesh = mesh::torus(alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), kFacet, kFacet, 0.5f, kExtent);
	insert_primitive(primitive_model::torus, mesh, alloc, io_alloc);

	mesh = mesh::box(alloc, io_alloc, mesh::face_type::outline, outline_layout, float3(-kExtent), float3(kExtent));
	insert_primitive(primitive_model::box_outline, mesh, alloc, io_alloc);

	mesh = mesh::circle(alloc, io_alloc, mesh::face_type::outline, outline_layout, kFacet, kExtent);
	insert_primitive(primitive_model::circle_outline, mesh, alloc, io_alloc);

	mesh = mesh::cylinder(alloc, io_alloc, mesh::face_type::outline, outline_layout, kFacet, 1, kExtent, 0.0f, 1.0f);
	insert_primitive(primitive_model::cone_outline, mesh, alloc, io_alloc);

	mesh = mesh::cylinder(alloc, io_alloc, mesh::face_type::outline, outline_layout, kFacet, 1, kExtent, kExtent, 2.0f);
	insert_primitive(primitive_model::cylinder_outline, mesh, alloc, io_alloc);

	mesh = mesh::circle(alloc, io_alloc, mesh::face_type::outline, outline_layout, 4, kExtent);
	mesh.bake_transform(quad_tx);
	insert_primitive(primitive_model::rectangle_outline, mesh, alloc, io_alloc);

	mesh = mesh::sphere(alloc, io_alloc, mesh::face_type::outline, outline_layout, kExtent);
	insert_primitive(primitive_model::sphere_outline, mesh, alloc, io_alloc);

	mesh = mesh::torus(alloc, io_alloc, mesh::face_type::outline, vlayout.data(), vlayout.size(), kFacet, kFacet, 0.5f, kExtent);
	insert_primitive(primitive_model::torus_outline, mesh, alloc, io_alloc);
}

model_registry::handle model_registry::load(const uri_t& uri_ref, const mesh::model& model)
{
	auto io_alloc = get_io_allocator();
	auto blob = mesh::encode(model, mesh::file_format::omdl, io_alloc, io_alloc);
	return insert(uri_ref, nullptr, blob, true);
}

}}

