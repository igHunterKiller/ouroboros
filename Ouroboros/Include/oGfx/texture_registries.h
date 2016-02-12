// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// handles I/O and referencing of textures

#pragma once
#include <oGfx/device_resource_registry.h>
#include <oGPU/gpu.h>

namespace ouro { namespace gfx {

struct texture2d_internal
{
	ref<gpu::srv> view;
};

typedef resource_t<texture2d_internal> texture2d_t;

class texture2d_registry : public device_resource_registry_t<texture2d_internal>
{
public:
  // The io_allocator is used to allocate placeholders and temporarily using load()
	// Placeholder memory will be freed before initialize() returns.
  void initialize(gpu::device* dev, uint32_t bookkeeping_bytes, const allocator& alloc, const allocator& io_alloc);
	void deinitialize();

  texture2d_t load(const path_t& path);
  void unload(const texture2d_t& tex);

  uint32_t flush(uint32_t max_operations = ~0u) { return device_resource_registry_t<basic_resource_type>::flush(max_operations); }

private:
  void* create(const char* name, blob& compiled) override;
  void destroy(void* resource) override;
};

}}
