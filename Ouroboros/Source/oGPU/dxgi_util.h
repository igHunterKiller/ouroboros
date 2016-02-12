// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oCore/ref.h>
#include <oSystem/adapter.h>
#include <oSurface/surface.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>

struct ID3D11RenderTargetView;

namespace ouro { namespace gpu { namespace dxgi {

// returns surface::unknown if there is no conversion
surface::format to_surface_format(DXGI_FORMAT format);

// supported this will return DXGI_FORMAT_UNKNOWN.
DXGI_FORMAT from_surface_format(const surface::format& format);

// returns true if a D* format
bool is_depth(DXGI_FORMAT format);

// returns true if a BC* format
bool is_block_compressed(DXGI_FORMAT format);

// returns pixel size or (block size for BC types) of format
uint32_t get_size(DXGI_FORMAT format, uint32_t plane = 0);

// dereferences an adapter id
ref<IDXGIAdapter> get_adapter(const adapter::id& adapter_id);

// returns an info for the specified adapter.
adapter::info_t get_info(IDXGIAdapter* adapter);

// A wrapper around CreateSwapChain that simplifies the input a bit.
ref<IDXGISwapChain> make_swap_chain(IUnknown* device
	, bool fullscreen
	, uint32_t width
	, uint32_t height
	, bool auto_change_monitor_resolution
	, const surface::format& format
	, uint32_t refresh_rate_numerator
	, uint32_t refresh_rate_denominator
	, HWND hwnd
	, bool enable_gdi_compatibility);

// Calls resize_buffers with new_size, but all other parameters from GetDesc.
// Also this encapsulates some robust/standard error reporting.
void resize_buffers(IDXGISwapChain* swapchain, const int2& new_size);
	
// Locks and returns the HDC for the render target associated with the 
// specified swap chain. Don't render while this HDC is valid! Call release
// when finished.
HDC acquire_dc(IDXGISwapChain* swapchain);
void release_dc(IDXGISwapChain* swapchain, RECT* dirty_rect = nullptr);
HDC acquire_dc(ID3D11RenderTargetView* rtv);
void release_dc(ID3D11RenderTargetView* rtv, RECT* dirty_rect = nullptr);

// The various formats required for getting a depth buffer to happen are 
// complicated, so centralize the mapping in this utility function. If the 
// Desired format is not a depth format, then desired_format is assigned as a 
// pass-thru to all three output values.
void get_compatible_formats(DXGI_FORMAT desired_format, DXGI_FORMAT* out_texture_format = nullptr, DXGI_FORMAT* out_dsv_format = nullptr, DXGI_FORMAT* out_srv_format = nullptr);

// Does some extra sanity checking
void set_fullscreen_exclusive(IDXGISwapChain* swapchain, bool fullscreen_exclusive);

// Does some extra sanity checking
void present(IDXGISwapChain* swapchain, uint32_t interval);

}}}
