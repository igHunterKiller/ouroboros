// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// handles I/O and referencing of models

#pragma once
#include <oGfx/device_resource_registry2.h>
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

typedef typed_resource<mesh::model> model_t;

class model_registry : public device_resource_registry2_t<mesh::model>
{
public:
	static const mesh::face_type face_type = mesh::face_type::front_cw;

  void initialize(gpu::device* dev, uint32_t budget_bytes, const allocator& alloc, const allocator& io_alloc);
	void deinitialize();

  model_t load(const path_t& path);
	model_t load(const path_t& path, const mesh::model& model);

  uint32_t flush(uint32_t max_operations = ~0u) { return device_resource_registry2_t<basic_resource_type>::flush(max_operations); }

	mesh::model* primitive(const primitive_model& prim) const { return resolve_indexed((uint64_t)prim + 1); }

	static void set_model(gpu::graphics_command_list* cl, const mesh::model* model);
	static void set_model(gpu::graphics_command_list* cl, const model_t& model) { set_model(cl, model.get()); }

	gfx::vertex_layout vertex_layout() const { return vertex_layout_; }
	gfx::vertex_shader vertex_shader() const { return vertex_shader_; }

private:
	allocator alloc_;

	void* create(const path_t& path, blob& compiled);
  void destroy(void* entry);

	void insert_primitive(const primitive_model& prim, const mesh::model& model, const allocator& alloc, const allocator& io_alloc);
	void insert_primitives(const allocator& alloc, const allocator& io_alloc);

	gfx::vertex_layout vertex_layout_;
	gfx::vertex_shader vertex_shader_;
};

}}