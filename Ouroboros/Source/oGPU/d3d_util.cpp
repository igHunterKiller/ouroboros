// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "d3d_util.h"
#include "dxgi_util.h"
#include <oCore/bit.h>
#include <oCore/stringf.h>
#include <oCore/version.h>
#include <oSystem/windows/win_util.h>
#include <d3d11_1.h>

namespace ouro { namespace gpu { namespace d3d {

ref<ID3D11Device> make_device(const device_init& init)
{
	if (init.api_version < version_t(9,0))
		throw std::invalid_argument("must be D3D 9.0 or above");

	ref<IDXGIAdapter> adapter;

	if (!init.use_software_emulation)
	{
		auto adapter_info = adapter::find(int2(init.virtual_desktop_x, init.virtual_desktop_y), init.min_driver_version, init.use_exact_driver_version);
		adapter = dxgi::get_adapter(adapter_info.id);
	}

	uint32_t flags = 0;
	bool debug = false;
	if (init.enable_driver_reporting)
	{
		flags |= D3D11_CREATE_DEVICE_DEBUG;
		debug = true;
	}

	if (!init.multithreaded)
		flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;

	ref<ID3D11Device> d3d;
	D3D_FEATURE_LEVEL feature_level;
	HRESULT hr = D3D11CreateDevice(
		adapter
		, init.use_software_emulation ? D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_UNKNOWN
		, 0
		, flags
		, nullptr
		, 0
		, D3D11_SDK_VERSION
		, &d3d
		, &feature_level
		, nullptr);

	// http://stackoverflow.com/questions/10586956/what-can-cause-d3d11createdevice-to-fail-with-e-fail
	// It's possible that the debug lib isn't installed, so try again without debug.

	if (hr == 0x887a002d)
	{
		#if NTDDI_VERSION < NTDDI_WIN8
			oTRACE("The DirectX SDK must be installed for driver-level debugging.");
		#else
			oTRACE("The Windows SDK must be installed for driver-level debugging.");
		#endif

		flags &=~ D3D11_CREATE_DEVICE_DEBUG;
		debug = false;
		hr = E_FAIL;
	}

	if (hr == E_FAIL)
	{
		oTRACE("The first-chance _com_error exception above is because there is no debug layer present during the creation of a D3D device, trying again without debug");

		flags &=~ D3D11_CREATE_DEVICE_DEBUG;
		debug = false;

		oV(D3D11CreateDevice(
			adapter
			, init.use_software_emulation ? D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_UNKNOWN
			, 0
			, flags
			, nullptr
			, 0
			, D3D11_SDK_VERSION
			, &d3d
			, &feature_level
			, nullptr));

		oTRACE("Debug D3D11 not found: device created in non-debug mode so driver error reporting will not be available.");
	}
	else
		oV(hr);

	version_t D3DVersion = version_t((feature_level>>12) & 0xffff, (feature_level>>8) & 0xffff);
	if (D3DVersion < init.api_version)
	{
		sstring strver;
		throw std::system_error(std::errc::not_supported, std::system_category(), std::string("Failed to create an ID3D11Device with a minimum feature set of D3D ") + to_string(strver, init.api_version));
	}

	debug_name(d3d, oSAFESTRN(init.name));

	if (debug && init.enable_driver_reporting)
	{
		ref<ID3D11InfoQueue> q;
		oV(d3d->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&q));

		q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
		q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
		q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, true);
		q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_INFO, true);
		q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_MESSAGE, true);
		q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
		q->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
	}

	return d3d;
}

gpu::device_desc get_desc(ID3D11Device* dev, bool is_software_emulation)
{
	gpu::device_desc d;
	debug_name(d.name, dev);

	ref<IDXGIAdapter> adapter;
	{
		ref<IDXGIDevice> dxgi;
		oV(dev->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgi));
		dxgi->GetAdapter(&adapter);
	}

	DXGI_ADAPTER_DESC ad;
	adapter->GetDesc(&ad);
	auto adapter_info = dxgi::get_info(adapter);

	ref<ID3D11DeviceContext> imm;
	dev->GetImmediateContext(&imm);

	bool profiler_attached = false;
	
	ref<ID3DUserDefinedAnnotation> anno;
	if (SUCCEEDED(dev->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&anno)))
		profiler_attached = !!anno->GetStatus();

	d.device_description = ad.Description;
	d.driver_description = adapter_info.description;
	d.native_memory = ad.DedicatedVideoMemory;
	d.dedicated_system_memory = ad.DedicatedSystemMemory;
	d.shared_system_memory = ad.SharedSystemMemory;
	d.driver_version = adapter_info.version;
	d.feature_version = adapter_info.feature_level;
	d.adapter_index = *(int*)&adapter_info.id;
	d.api = gpu_api::d3d11;
	d.vendor = adapter_info.vendor;
	d.is_software_emulation = is_software_emulation;
	d.driver_reporting_enabled = !!(dev->GetCreationFlags() & D3D11_CREATE_DEVICE_DEBUG);
	d.profiler_attached = profiler_attached;
	return d;
}

bool supports_deferred_contexts(ID3D11Device* dev)
{
	D3D11_FEATURE_DATA_THREADING threading_caps = { FALSE, FALSE };
	oV(dev->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threading_caps, sizeof(threading_caps)));
	return !!threading_caps.DriverCommandLists;
}

uint32_t bytecode_size(const void* bytecode)
{
	// Discovered empirically
	return bytecode ? ((const uint32_t*)bytecode)[6] : 0;
}

void set_samplers(ID3D11DeviceContext* dc, uint32_t sampler_start, uint32_t num_samplers, ID3D11SamplerState* const* samplers, uint32_t stage_bindings)
{
	if (stage_bindings & stage_binding::vertex)   dc->VSSetSamplers(sampler_start, num_samplers, samplers);
	if (stage_bindings & stage_binding::hull)     dc->HSSetSamplers(sampler_start, num_samplers, samplers);
	if (stage_bindings & stage_binding::domain)   dc->DSSetSamplers(sampler_start, num_samplers, samplers);
	if (stage_bindings & stage_binding::geometry) dc->GSSetSamplers(sampler_start, num_samplers, samplers);
	if (stage_bindings & stage_binding::pixel)    dc->PSSetSamplers(sampler_start, num_samplers, samplers);
	if (stage_bindings & stage_binding::compute)  dc->CSSetSamplers(sampler_start, num_samplers, samplers);
}

void set_cbuffers(ID3D11DeviceContext* dc, uint32_t cbuffer_start, uint32_t num_cbuffers, ID3D11Buffer* const* buffers, uint32_t stage_bindings)
{
	if (stage_bindings & stage_binding::vertex)   dc->VSSetConstantBuffers(cbuffer_start, num_cbuffers, buffers);
	if (stage_bindings & stage_binding::hull)     dc->HSSetConstantBuffers(cbuffer_start, num_cbuffers, buffers);
	if (stage_bindings & stage_binding::domain)   dc->DSSetConstantBuffers(cbuffer_start, num_cbuffers, buffers);
	if (stage_bindings & stage_binding::geometry) dc->GSSetConstantBuffers(cbuffer_start, num_cbuffers, buffers);
	if (stage_bindings & stage_binding::pixel)    dc->PSSetConstantBuffers(cbuffer_start, num_cbuffers, buffers);
	if (stage_bindings & stage_binding::compute)  dc->CSSetConstantBuffers(cbuffer_start, num_cbuffers, buffers);
}

static bool has_unordered_buffers(uint32_t num_srvs, ID3D11ShaderResourceView* const* srvs)
{
	if (!srvs)
		return false;

	for (uint32_t i = 0; i < num_srvs; i++)
	{
		if (!srvs[i])
			continue;
		
		ref<ID3D11UnorderedAccessView> uav;
		if (SUCCEEDED(srvs[i]->QueryInterface(IID_PPV_ARGS(&uav))))
			return true;
	}
	
	return false;
}

void set_srvs(ID3D11DeviceContext* dc, uint32_t srv_start, uint32_t num_srvs, ID3D11ShaderResourceView* const* srvs, uint32_t stage_bindings)
{
	if (has_unordered_buffers(num_srvs, srvs))
		stage_bindings &= ~(stage_binding::vertex|stage_binding::hull|stage_binding::domain|stage_binding::geometry);
	if (stage_bindings & stage_binding::vertex)   dc->VSSetShaderResources(srv_start, num_srvs, srvs);
	if (stage_bindings & stage_binding::hull)     dc->HSSetShaderResources(srv_start, num_srvs, srvs);
	if (stage_bindings & stage_binding::domain)   dc->DSSetShaderResources(srv_start, num_srvs, srvs);
	if (stage_bindings & stage_binding::geometry) dc->GSSetShaderResources(srv_start, num_srvs, srvs);
	if (stage_bindings & stage_binding::pixel)    dc->PSSetShaderResources(srv_start, num_srvs, srvs);
	if (stage_bindings & stage_binding::compute)  dc->CSSetShaderResources(srv_start, num_srvs, srvs);
}

bool copy_async_data(ID3D11Device* dev, ID3D11Asynchronous* async, void* dst, uint32_t dst_size, bool blocking)
{
	ref<ID3D11DeviceContext> dc;
	dev->GetImmediateContext(&dc);

	backoff bo;
	HRESULT hr = S_FALSE;

	while(true)
	{
		hr = dc->GetData(async, dst, dst_size, 0);
		if (S_FALSE != hr || !blocking)
			break;
		bo.pause();
	}

	return SUCCEEDED(hr);
}

template<typename ResourceDescT>
static void set_staging(ResourceDescT* desc)
{
	desc->Usage = D3D11_USAGE_STAGING;
	desc->CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc->BindFlags = 0;
	desc->MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE; // keep cube flag because that's the only way to differentiate between 2D and cube types
}

ref<ID3D11Resource> make_cpu_copy(ID3D11Resource* src, bool do_copy, uint32_t* out_bytes)
{
	ref<ID3D11Device> dev;
	src->GetDevice(&dev);
	ref<ID3D11DeviceContext> dc;
	dev->GetImmediateContext(&dc);

	ref<ID3D11Resource> copy;

	uint32_t bytes = 0;
	D3D11_RESOURCE_DIMENSION type = D3D11_RESOURCE_DIMENSION_UNKNOWN;
	src->GetType(&type);

	switch (type)
	{
		case D3D11_RESOURCE_DIMENSION_BUFFER:
		{
			D3D11_BUFFER_DESC desc;
			((ID3D11Buffer*)src)->GetDesc(&desc);
			bytes = desc.ByteWidth;
			set_staging(&desc);
			oV(dev->CreateBuffer(&desc, nullptr, (ID3D11Buffer**)&copy));
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
		{
			D3D11_TEXTURE1D_DESC desc;
			((ID3D11Texture1D*)src)->GetDesc(&desc);
			bytes = desc.Width * desc.ArraySize * dxgi::get_size(desc.Format);
			set_staging(&desc);
			oV(dev->CreateTexture1D(&desc, nullptr, (ID3D11Texture1D**)&copy));
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
		{
			D3D11_TEXTURE2D_DESC desc;
			((ID3D11Texture2D*)src)->GetDesc(&desc);
			bytes = desc.Width * desc.Height * desc.ArraySize * dxgi::get_size(desc.Format);
			set_staging(&desc);
			oV(dev->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&copy));
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
		{
			D3D11_TEXTURE3D_DESC desc;
			((ID3D11Texture3D*)src)->GetDesc(&desc);
			bytes = desc.Width * desc.Height * desc.Depth * dxgi::get_size(desc.Format);
			set_staging(&desc);
			oV(dev->CreateTexture3D(&desc, nullptr, (ID3D11Texture3D**)&copy));
			break;
		}

		default:
			throw std::invalid_argument("unknown resource type");
	}

	// set the debug name
	{
		char res_name[64];
		debug_name(res_name, src);

		char copy_name[64];
		snprintf(copy_name, "%s.cpu_copy", res_name);

		debug_name(copy, copy_name);
	}

	if (do_copy)
	{
		dc->CopyResource(copy, src);
		dc->Flush();
	}

	if (out_bytes)
		*out_bytes = bytes;

	return copy;
}

surface::image make_snapshot(ID3D11Texture2D* tex, bool flip_vertically, const allocator& alloc)
{
	if (!tex)
		throw std::invalid_argument("invalid texture");

	ref<ID3D11Resource> copy = make_cpu_copy(tex);

	D3D11_TEXTURE2D_DESC desc;
	tex->GetDesc(&desc);
	if (desc.Format == DXGI_FORMAT_UNKNOWN)
		throw std::system_error(std::errc::not_supported, std::system_category(), std::string("The specified texture's format ") + as_string(desc.Format) + " is not supported by surface::image");

	surface::info_t si;
	si.format = dxgi::to_surface_format(desc.Format);
	si.dimensions = int3(desc.Width, desc.Height, 1);
	surface::image s(si, alloc);

	ref<ID3D11Device> dev;
	tex->GetDevice(&dev);
	ref<ID3D11DeviceContext> dc;
	dev->GetImmediateContext(&dc);

	D3D11_MAPPED_SUBRESOURCE mapped;
	oV(dc->Map(copy, 0, D3D11_MAP_READ, 0, &mapped));
	surface::lock_guard lock(s);
	memcpy2d(lock.mapped.data, lock.mapped.row_pitch, mapped.pData, mapped.RowPitch, lock.byte_dimensions.x, lock.byte_dimensions.y, flip_vertically);
	dc->Unmap(copy, 0);

	return s;
}

// grow by increasingly larger quanta as capacity gets larger
static uint32_t calc_next_capacity(uint32_t cur_capacity)
{
	return max(1u, log2i(cur_capacity) - 2u);
}

void fitted_cbuffer::initialize(const char* name, ID3D11Device* dev, uint32_t struct_stride, uint32_t max_num_structs)
{
	max_num_structs = max(2u, max_num_structs); // a min of 2 squelches a D3D warning

	struct_stride_ = struct_stride;

	// count how many buffers will be needed to allocate an array
	num_cbuffers_ = 0;
	uint32_t nstructs = 0;
	while (nstructs < max_num_structs)
	{
		num_cbuffers_++;
		nstructs = calc_next_capacity(nstructs);
	}

	if (num_cbuffers_ >= cbuffers_.size())
		throw std::overflow_error(stringf("too many cbuffers (%u) required (max %u)", num_cbuffers_, cbuffers_.size()));

	size_t len = strlen(name) + 1 + 5; // add some room for numbers
	char* buf = (char*)alloca(len);
	int offset = snprintf(buf, len, "%s", name);
	char* num = buf + offset;
	size_t num_len = len - offset;

	for (uint32_t i = 0, n = 1; i < num_cbuffers_; i++, n = calc_next_capacity(n))
	{
		CD3D11_BUFFER_DESC desc(struct_stride * n, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, 0, struct_stride);
		oV(dev->CreateBuffer(&desc, nullptr, &cbuffers_[i]));
		snprintf(num, num_len, "%s%u", name, n);
		debug_name(buf, len, cbuffers_[i]);
	}
	
	// ensure there's a cbuffer that holds exactly the requested max number of structs
	if (nstructs != max_num_structs)
	{
		CD3D11_BUFFER_DESC desc(struct_stride * max_num_structs, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE, 0, struct_stride);
		oV(dev->CreateBuffer(&desc, nullptr, &cbuffers_[num_cbuffers_]));
		snprintf(num, num_len, "%s%u", name, max_num_structs);
		debug_name(buf, len, cbuffers_[num_cbuffers_]);
		num_cbuffers_++;
	}
}

ID3D11Buffer* fitted_cbuffer::best_fit(uint32_t num_structs) const
{
	if (num_structs)
	{
		num_structs = max(2u, num_structs); // a min of 2 squelches a D3D warning
		for (uint32_t i = 0, n = 1; i < num_cbuffers_; i++, n = calc_next_capacity(n))
			if (n >= num_structs)
				return (ID3D11Buffer*)cbuffers_[i].c_ptr();

		throw std::out_of_range("too many structs requested");
	}

	return nullptr;
}

void debug_name(ID3D11Device* dev, const char* name)
{
	if (dev && name)
	{
		uint32_t CreationFlags = dev->GetCreationFlags();
		if (CreationFlags & D3D11_CREATE_DEVICE_DEBUG)
		{
			sstring Buffer(name); // if strings aren't the same size D3D issues a warning
			oV(dev->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(Buffer.capacity()), Buffer.c_str()));
		}
	}
}

char* debug_name(char* dst, size_t dst_size, const ID3D11Device* dev)
{
	uint32_t size = (uint32_t)dst_size;
	uint32_t CreationFlags = const_cast<ID3D11Device*>(dev)->GetCreationFlags();
	if (CreationFlags & D3D11_CREATE_DEVICE_DEBUG)
		oV(const_cast<ID3D11Device*>(dev)->GetPrivateData(WKPDID_D3DDebugObjectName, &size, dst));
	else if (strlcpy(dst, "non-debug dev", dst_size) >= dst_size)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return dst;
}

void debug_name(ID3D11DeviceChild* child, const char* name)
{
	if (child && name)
	{
		ref<ID3D11Device> dev;
		child->GetDevice(&dev);
		uint32_t CreationFlags = dev->GetCreationFlags();
		if (CreationFlags & D3D11_CREATE_DEVICE_DEBUG)
		{
			sstring Buffer(name); // if strings aren't the same size, D3D issues a warning
			oV(child->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(Buffer.capacity()), Buffer.c_str()));
		}
	}
}

char* debug_name(char* dst, size_t dst_size, const ID3D11DeviceChild* child)
{
	uint32_t size = (uint32_t)dst_size;
	ref<ID3D11Device> dev;
	const_cast<ID3D11DeviceChild*>(child)->GetDevice(&dev);
	uint32_t CreationFlags = dev->GetCreationFlags();
	if (CreationFlags & D3D11_CREATE_DEVICE_DEBUG)
	{
		HRESULT hr = const_cast<ID3D11DeviceChild*>(child)->GetPrivateData(WKPDID_D3DDebugObjectName, &size, dst);
		if (hr == DXGI_ERROR_NOT_FOUND)
			strlcpy(dst, "unnamed", dst_size);
		else
			oV(hr);
	}
	else if (strlcpy(dst, "non-debug dev child", dst_size) >= dst_size)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return dst;
}

int vtrace(ID3D11InfoQueue* iq, D3D11_MESSAGE_SEVERITY severity, const char* format, va_list args)
{
	xlstring buf;
	int len = vsnprintf(buf, format, args);
	if (len == -1)
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	iq->AddApplicationMessage(severity, buf);
	return len;
}

}}}
