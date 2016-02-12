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

typedef resource_t<mesh::model> model_t;

class model_registry : protected device_resource_registry_t<mesh::model>
{
public:
	static const mesh::face_type face_type = mesh::face_type::front_cw;

  // The io_allocator is used to allocate placeholders and temporarily using load()
	// Placeholder memory will be freed before initialize() returns. The memory used
	// by this class is only for bookkeeping, not for the geometry payload. That is
	// allocated from device memory.
  void initialize(gpu::device* dev, uint32_t bookkeeping_bytes, const allocator& alloc, const allocator& io_alloc);

	void deinitialize();

  model_t load(const path_t& path);
	model_t load(const path_t& path, const mesh::model& model);
  void unload(const model_t& mdl);

	model_t primitive(const primitive_model& prim) const { return get((key_type)prim + 1); }

	static void set_model(gpu::graphics_command_list* cl, const model_t& model);

  uint32_t flush(uint32_t max_operations = ~0u) { return device_resource_registry_t<mesh::model>::flush(max_operations); }

	gfx::vertex_layout vertex_layout() const { return vertex_layout_; }
	gfx::vertex_shader vertex_shader() const { return vertex_shader_; }

private:
  void* create(const char* name, blob& compiled);
  void destroy(void* entry);

	void insert_primitive(const primitive_model& prim, const mesh::model& model, const allocator& alloc, const allocator& io_alloc);
	void insert_primitives(const allocator& alloc, const allocator& io_alloc);
	void remove_primitives();

	gfx::vertex_layout vertex_layout_;
	gfx::vertex_shader vertex_shader_;
};

}}
