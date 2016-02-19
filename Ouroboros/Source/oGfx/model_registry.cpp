// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/model_registry.h>
#include <oMath/matrix.h>
#include <oMesh/codec.h>
#include <oMesh/obj.h>
#include <oMesh/primitive.h>
#include <oString/stringize.h>

namespace ouro { 

const char* as_string(const gfx::primitive_model& m)
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

	return detail::counted_enum_as_string(m, s_names);
}
	
namespace gfx {

void model_registry::initialize(gpu::device* dev, uint32_t bookkeeping_bytes, const allocator& alloc, const allocator& io_alloc)
{
	vertex_layout_ = vertex_layout::pos_nrm_tan_uv0_col;
	vertex_shader_ = vertex_shader::pos_nrm_tan_uv0_col;

	auto vlayout = gfx::layout(vertex_layout_);

	// if there's a static primitives registry, this might be able to turn into an initialize_with_builtin
	mesh::model placeholder = mesh::box(io_alloc, io_alloc, face_type, vlayout.data(), vlayout.size(), float3(-1.0f), float3(1.0f), (uint32_t)color::default_gray);

	auto placeholder_file = mesh::encode(placeholder, mesh::file_format::omdl, io_alloc, io_alloc);

  resource_placeholders_t placeholders;
	placeholders.compiled_missing = placeholder_file.alias();
  placeholders.compiled_loading = placeholder_file.alias();
  placeholders.compiled_failed = placeholder_file.alias();

  initialize_base(dev, "model_registry", bookkeeping_bytes, alloc, io_alloc, placeholders);
	insert_primitives(alloc, io_alloc);

	flush();
}

void model_registry::deinitialize()
{
	if (!valid())
		return;

	remove_primitives();
	deinitialize_base();
}

void* model_registry::create(const char* name, blob& compiled)
{
	// todo: find more meaningful allocators here
	const allocator& subsets_allocator = default_allocator;
	const allocator& temp_allocator = default_allocator;

	auto fformat = mesh::get_file_format(compiled);
	oCheck(fformat != mesh::file_format::unknown, std::errc::invalid_argument, "Unknown file format: %s", name ? name : "(null)");

	auto mdl = pool_.create();

	auto vlayout = gfx::layout(vertex_layout_);

	// todo: figure out a way that this can allocate right out of the cpu-accessible buffer... or hope for D3D12
	*mdl = mesh::decode(compiled, vlayout, subsets_allocator, temp_allocator, temp_allocator);
	auto info = mdl->info();

	gpu::ibv indices = dev_->new_ibv(name, info.num_indices, mdl->indices());

	std::array<uint32_t, mesh::max_num_slots> vertices_offsets;
	vertices_offsets.fill(0);

	const uint32_t nslots = info.num_slots;
	for (uint32_t slot = 0; slot < nslots; slot++)
	{
		const auto stride = mdl->vertex_stride(slot);
		gpu::vbv verts = dev_->new_vbv(name, stride, info.num_vertices, mdl->vertices(slot));
		vertices_offsets[slot] = verts.offset;
	}

	mdl->rebase(dev_->readable_mesh_base(), noop_deallocate, indices.offset, vertices_offsets);

	return mdl;
}

void model_registry::destroy(void* entry)
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

void model_registry::set_model(gpu::graphics_command_list* cl, const model_t& mdl)
{
	auto& info = mdl->info();
	
	// Reconstruct ibv and bind it
	{
		gpu::ibv indices;
		indices.offset = mdl->indices_offset();
		indices.num_indices = info.num_indices;
		indices.transient = 0;
		indices.is_32bit = false;
		
		cl->set_indices(indices);
	}

	// Reconstruct vbvs and bind them
	auto nslots = info.num_slots;
	gpu::vbv vbvs[mesh::max_num_slots];
	for (uint32_t slot = 0; slot < nslots; slot++)
	{
		auto stride = mdl->vertex_stride(slot);

		auto& verts = vbvs[slot];
		verts.offset = mdl->vertices_offset(slot);
		verts.num_vertices = info.num_vertices;
		verts.transient = 0;
		verts.vertex_stride_bytes(mdl->vertex_stride(slot));
	}

	cl->set_vertices(0, nslots, vbvs);
}

void model_registry::insert_primitive(const primitive_model& prim, const mesh::model& model, const allocator& alloc, const allocator& io_alloc)
{
	// reminder: +1's on key is because 0/null is a reserved symbol
	auto omdl = mesh::encode(model, mesh::file_format::omdl, alloc, io_alloc);
	insert((key_type)prim + 1, as_string(prim), omdl);
}

void model_registry::insert_primitives(const allocator& alloc, const allocator& io_alloc)
{
	auto vlayout = gfx::layout(vertex_layout_);
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

void model_registry::remove_primitives()
{
	for (key_type prim_key = 1; prim_key <= (key_type)primitive_model::count; prim_key++)
		remove(prim_key);
}

model_t model_registry::load(const path_t& path)
{
	return default_load(path);
}

model_t model_registry::load(const path_t& path, const mesh::model& model)
{
	auto blob = mesh::encode(model, mesh::file_format::omdl, io_alloc_, io_alloc_);
	return insert(path, blob, true);
}

void model_registry::unload(const model_t& mdl)
{
  remove(mdl);
}

}}
