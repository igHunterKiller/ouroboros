// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// handles I/O and referencing of textures

#pragma once
#include <oGfx/device_resource_registry2.h>
#include <oGPU/gpu.h>

namespace ouro { namespace gfx {

struct texture2d_internal
{
	ref<gpu::srv> view;
};

typedef typed_resource<texture2d_internal> texture2d2_t;

class texture2d_registry2 : public device_resource_registry2_t<texture2d_internal>
{
public:

	static const memory_alignment required_alignment = device_resource_registry2_t<texture2d_internal>::required_alignment;

	~texture2d_registry2() {}

  void initialize(gpu::device* dev, uint32_t budget_bytes, const allocator& alloc, const allocator& io_alloc);
	void deinitialize();

  texture2d2_t load(const path_t& path);

  uint32_t flush(uint32_t max_operations = ~0u) { return device_resource_registry2_t<basic_resource_type>::flush(max_operations); }

private:
	allocator alloc_;
  
	void* create(const path_t& path, blob& compiled) override;
  void destroy(void* resource) override;
};

}}
