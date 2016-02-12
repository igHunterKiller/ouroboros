// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oCore/ref.h>
#include <oSystem/windows/win_error.h>
#include <oGPU/gpu.h>
#include <oSurface/surface.h>
#include <d3d11_1.h>

namespace ouro { namespace gpu { namespace d3d {

// _____________________________________________________________________________
// Creation/inspection utilities
	
ref<ID3D11Device> make_device(const gpu::device_init& init);

// returns info about dev. (there's no way to determine if the device is software, so pass that through)
device_desc get_desc(ID3D11Device* dev, bool is_software_emulation);

// http://msdn.microsoft.com/en-us/library/windows/desktop/ff476486(v=vs.85).aspx
// This is not cheap enough to reevaluate for each call to update_subresource, so
// call this once and cache the result per device and pass it to update_subresource
// as appropriate.
bool supports_deferred_contexts(ID3D11Device* dev);

// return the size of the specified hlsl shader bytecode
uint32_t bytecode_size(const void* bytecode);

// _____________________________________________________________________________
// State simplification

void set_samplers(ID3D11DeviceContext* dc, uint32_t sampler_start, uint32_t num_samplers, ID3D11SamplerState* const* samplers, uint32_t stage_bindings);
void set_cbuffers(ID3D11DeviceContext* dc, uint32_t cbuffer_start, uint32_t num_cbuffers, ID3D11Buffer* const* buffers, uint32_t stage_bindings);
void set_srvs(ID3D11DeviceContext* dc, uint32_t srv_start, uint32_t num_srvs, ID3D11ShaderResourceView* const* srvs, uint32_t stage_bindings);

// _____________________________________________________________________________
// Advanced copying

// blocks until async's data is available and can be read into dst
bool copy_async_data(ID3D11Device* dev, ID3D11Asynchronous* async, void* dst, uint32_t dst_size, bool blocking = true);
template<typename T> bool copy_async_data(ID3D11Device* dev, ID3D11Asynchronous* async, T* data, bool blocking = true) { return copy_async_data(dev, async, data, sizeof(T), blocking); }

// Creates a STAGING copy of src, does the copy and flushes the immediate context.
// If do_copy is false then the buffer is created uninitialized. If out_bytes is 
// valid it will receive the size of the bulk data associated with the new resource.
ref<ID3D11Resource> make_cpu_copy(ID3D11Resource* src, bool do_copy = true, uint32_t* out_bytes = nullptr);

// copies the contents of t to a new surface buffer
surface::image make_snapshot(ID3D11Texture2D* tex, bool flip_vertically = false, const allocator& alloc = default_allocator);

// cbuffers are updated wholly each time. For variable-length arrays of structs 
// such as per-draw structs for instances, there's no good way to reuse the same 
// buffer but only update a subset, so allocate several sizes of cbuffer and 
// return the best-fit.
class fitted_cbuffer
{
public:
	fitted_cbuffer()
		: num_cbuffers_(0)
		, struct_stride_(0)
	{}

  void initialize(const char* name, ID3D11Device* dev, uint32_t struct_stride, uint32_t max_num_structs);

	ID3D11Buffer* best_fit(uint32_t num_structs) const;

	uint32_t struct_stride() const { return struct_stride_; }

private:
	std::array<ref<ID3D11Buffer>, 128> cbuffers_;
	uint32_t num_cbuffers_;
	uint32_t struct_stride_;
};

// _____________________________________________________________________________
// Debug naming and tracing

// Set/Get a name that appears in D3D11's debug layer
void debug_name(ID3D11Device* dev, const char* name);
void debug_name(ID3D11DeviceChild* child, const char* name);

char* debug_name(char* dst, size_t dst_size, const ID3D11Device* dev);
template<size_t size> char* debug_name(char (&dst)[size], const ID3D11Device* dev) { return debug_name(dst, size, dev); }
template<size_t capacity> char* debug_name(fixed_string<char, capacity>& dst, const ID3D11Device* dev) { return debug_name(dst, dst.capacity(), dev); }

// Fills the specified buffer with the string set with debug_name().
char* debug_name(char* dst, size_t dst_size, const ID3D11DeviceChild* child);
template<size_t size> char* debug_name(char (&dst)[size], const ID3D11DeviceChild* child) { return debug_name(dst, size, child); }
template<size_t capacity> char* debug_name(fixed_string<char, capacity>& dst, const ID3D11DeviceChild* child) { return debug_name(dst, dst.capacity(), child); }

// Use InfoQueue::AddApplicationMessage to trace user errors.
int vtrace(ID3D11InfoQueue* iq, D3D11_MESSAGE_SEVERITY severity, const char* format, va_list args);
inline int vtrace(ID3D11Device* dev, D3D11_MESSAGE_SEVERITY severity, const char* format, va_list args)       { ref<ID3D11InfoQueue> InfoQueue; oV(dev->QueryInterface(__uuidof(InfoQueue), (void**)&InfoQueue)); return vtrace(InfoQueue, severity, format, args); }
inline int vtrace(ID3D11DeviceContext* dc, D3D11_MESSAGE_SEVERITY severity, const char* format, va_list args) { ref<ID3D11Device> Device; dc->GetDevice(&Device); return vtrace(Device, severity, format, args); }
inline int trace(ID3D11InfoQueue* iq, D3D11_MESSAGE_SEVERITY severity, const char* format, ...)               { va_list args; va_start(args, format); int len = vtrace(iq, severity, format, args); va_end(args); return len; }
inline int trace(ID3D11Device* dev, D3D11_MESSAGE_SEVERITY severity, const char* format, ...)                 { va_list args; va_start(args, format); int len = vtrace(dev, severity, format, args); va_end(args); return len; }
inline int trace(ID3D11DeviceContext* dc, D3D11_MESSAGE_SEVERITY severity, const char* format, ...)           { va_list args; va_start(args, format); int len = vtrace(dc, severity, format, args); va_end(args); return len; }

}}}
