// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// handles I/O and referencing of models

#pragma once
#include <oGfx/device_resource_registry.h>
#include <oGfx/bytecode.h>
#include <oMesh/model.h>

namespace ouro { namespace gfx {

enum class primitive_model : uint8_t
{
	// Use a model-compatible format
	box,
	circle,
	cone,
	cylinder,
	pyramid,
	rectangle,
	sphere,
	torus,

	// Use gfx::pipeline_state::lines_color
	box_outline,
	circle_outline,
	cone_outline,
	cylinder_outline,
	pyramid_outline,
	rectangle_outline,
	sphere_outline,
	torus_outline,

	count,
};

class model_registry : public device_resource_registry<mesh::model>
{
public:
	typedef device_resource_registry<mesh::model> base_t;
	typedef base_t::handle handle;

	static const mesh::face_type face_type = mesh::face_type::front_cw;

  void initialize(gpu::device* dev, uint32_t budget_bytes, const allocator& alloc, const allocator& io_alloc);
	void deinitialize();

	handle load(const uri_t& uri_ref, mesh::model* placeholder, bool force = false) { return base_t::load(uri_ref, placeholder, force); }
	handle load(const uri_t& uri_ref, const mesh::model& model);

  uint32_t flush(uint32_t max_operations = ~0u) { return base_t::flush(max_operations); }

	mesh::model* primitive(const primitive_model& prim) const { return resolve_indexed((uint64_t)prim + 1); }

	gfx::vertex_layout vertex_layout() const { return vertex_layout_; }
	gfx::vertex_shader vertex_shader() const { return vertex_shader_; }

private:
	allocator alloc_;

	void* create_resource(const uri_t& uri_ref, blob& compiled);
  void destroy_resource(void* entry);

	void insert_primitive(const primitive_model& prim, const mesh::model& model, const allocator& alloc, const allocator& io_alloc);
	void insert_primitives(const allocator& alloc, const allocator& io_alloc);

	gfx::vertex_layout vertex_layout_;
	gfx::vertex_shader vertex_shader_;
};

typedef model_registry::handle model_t;

}}