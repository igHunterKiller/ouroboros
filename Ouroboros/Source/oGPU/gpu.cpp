// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGPU/gpu.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/windows/win_util.h>
#include <oGUI/window.h>
#include <oGUI/windows/win_gdi.h>
#include <oGUI/windows/win_gdi_draw.h>
#include "d3d_util.h"
#include "dxgi_util.h"
#include <oCore/assert.h>

#define oGPU_CHECK(expr, format, ...) oCheck(expr, std::errc::invalid_argument, format, ## __VA_ARGS__)

using namespace ouro::gpu::d3d;

namespace ouro { 

template<> const char* as_string(const gpu::stage_binding::flag& binding)
{
	switch (binding)
	{
		case gpu::stage_binding::vertex:	 return "vertex";
		case gpu::stage_binding::hull:		 return "hull";
		case gpu::stage_binding::domain:	 return "domain";
		case gpu::stage_binding::geometry: return "geometry";
		case gpu::stage_binding::pixel:		 return "pixel";
		case gpu::stage_binding::compute:  return "compute";
	}

	return "?";
}
	
namespace gpu {

static void* s_null_memory[128];

namespace basic {

#include <VSfullscreen_tri.h>
#include <VSpass_through_pos.h>
#include <PSblack.h>
#include <PSwhite.h>
#include <PStex2d.h>

}

static const char* from_element_semantic(const mesh::element_semantic& s)
{
	switch (s)
	{
		case mesh::element_semantic::position: return "POSITION";
		case mesh::element_semantic::normal: return "NORMAL";
		case mesh::element_semantic::tangent: return "TANGENT";
		case mesh::element_semantic::texcoord: return "TEXCOORD";
		case mesh::element_semantic::color: return "COLOR";
		case mesh::element_semantic::misc: return "MISC";
		default: break;
	};
	return "?";
}

static D3D11_INPUT_ELEMENT_DESC from_input_element_desc(const mesh::element_t& e)
{
	D3D11_INPUT_ELEMENT_DESC d;
	d.SemanticName = from_element_semantic(e.semantic);
	d.SemanticIndex = e.index;
	d.Format = dxgi::from_surface_format(e.format);
	d.InputSlot = e.slot;
	d.AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	d.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	d.InstanceDataStepRate = 0;
	return d;
}

static D3D11_PRIMITIVE_TOPOLOGY from_primitive_type(const primitive_type& type)
{
	switch (type)
	{
		case primitive_type::point: return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
		case primitive_type::line: return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case primitive_type::triangle: return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case primitive_type::patch: return D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST;
	}
	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

static D3D11_CULL_MODE from_cull_mode(const cull_mode& mode)
{
	return (D3D11_CULL_MODE)((int)mode + 1);
}
	
static D3D11_RASTERIZER_DESC from_rasterizer_desc(const rasterizer_desc& desc)
{
	D3D11_RASTERIZER_DESC rs;
	rs.FillMode = desc.wireframe ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
	rs.CullMode = from_cull_mode(desc.cull_mode);
	rs.FrontCounterClockwise = desc.front_ccw;
	rs.DepthBias = desc.depth_bias;
	rs.DepthBiasClamp = desc.depth_bias_clamp;
	rs.SlopeScaledDepthBias = desc.slope_scaled_depth_bias;
	rs.DepthClipEnable = desc.depth_clip_enable;
	rs.ScissorEnable = desc.scissor_enable;
	rs.MultisampleEnable = desc.multisample_enable;
	rs.AntialiasedLineEnable = desc.antialiased_line_enabled;
	return rs;
}

static D3D11_BLEND from_blend_type(const blend_type& blend)
{
	static const D3D11_BLEND sBlends[] = 
	{
		D3D11_BLEND_ZERO,          D3D11_BLEND_ONE,
		D3D11_BLEND_SRC_COLOR,     D3D11_BLEND_INV_SRC_COLOR,
		D3D11_BLEND_SRC_ALPHA,     D3D11_BLEND_INV_SRC_ALPHA,
		D3D11_BLEND_DEST_ALPHA,    D3D11_BLEND_INV_DEST_ALPHA,
		D3D11_BLEND_DEST_COLOR,    D3D11_BLEND_INV_DEST_COLOR,
		D3D11_BLEND_SRC_ALPHA_SAT,
		D3D11_BLEND_BLEND_FACTOR,  D3D11_BLEND_INV_BLEND_FACTOR,
		D3D11_BLEND_SRC1_COLOR,    D3D11_BLEND_INV_SRC1_COLOR,
		D3D11_BLEND_SRC1_ALPHA,    D3D11_BLEND_INV_SRC1_ALPHA,
	};

	return sBlends[(int)blend];
}

static D3D11_BLEND_OP from_blend_op(const blend_op& op)
{
	return (D3D11_BLEND_OP)((int)op + 1);
}

static D3D11_BLEND_DESC from_blend_desc(const blend_desc& desc)
{
	D3D11_BLEND_DESC bs;
	bs.AlphaToCoverageEnable = desc.alpha_to_coverage_enable;
	bs.IndependentBlendEnable = desc.independent_blend_enable;
	for (int i = 0; i < 8; i++)
	{
		auto& bsrt = bs.RenderTarget[i];
		const auto& desc_rt = desc.render_targets[i];

		bsrt.BlendEnable = desc_rt.blend_enable;
    bsrt.SrcBlend = from_blend_type(desc_rt.src_blend_rgb);
    bsrt.DestBlend = from_blend_type(desc_rt.dest_blend_rgb);
    bsrt.BlendOp = from_blend_op(desc_rt.blend_op_rgb);
    bsrt.SrcBlendAlpha = from_blend_type(desc_rt.src_blend_alpha);
    bsrt.DestBlendAlpha = from_blend_type(desc_rt.dest_blend_alpha);
    bsrt.BlendOpAlpha = from_blend_op(desc_rt.blend_op_alpha);
    bsrt.RenderTargetWriteMask = desc_rt.render_target_write_mask;
	}
	return bs;
}

static D3D11_COMPARISON_FUNC from_comparison(const comparison& c)
{
	return (D3D11_COMPARISON_FUNC)((int)c + 1);
}

static D3D11_STENCIL_OP from_stencil_op(const stencil_op& op)
{
	return (D3D11_STENCIL_OP)((int)op + 1);
}

static D3D11_DEPTH_STENCIL_DESC from_depth_stencil_desc(const depth_stencil_desc& desc)
{
	D3D11_DEPTH_STENCIL_DESC ds;
	ds.DepthEnable = desc.depth_enable;
	ds.DepthWriteMask = desc.depth_write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	ds.DepthFunc = from_comparison(desc.depth_func);
	ds.StencilEnable = desc.stencil_enable;
	ds.StencilReadMask = desc.stencil_mask_read;
	ds.StencilWriteMask = desc.stencil_mask_write;
	ds.FrontFace.StencilFailOp = from_stencil_op(desc.stencil_front_face_fail_op);
	ds.FrontFace.StencilDepthFailOp = from_stencil_op(desc.stencil_front_face_depth_fail_op);
	ds.FrontFace.StencilPassOp = from_stencil_op(desc.stencil_front_face_pass_op);
	ds.FrontFace.StencilFunc = from_comparison(desc.stencil_front_face_func);
	ds.BackFace.StencilFailOp = from_stencil_op(desc.stencil_back_face_fail_op);
	ds.BackFace.StencilDepthFailOp = from_stencil_op(desc.stencil_back_face_depth_fail_op);
	ds.BackFace.StencilPassOp = from_stencil_op(desc.stencil_back_face_pass_op);
	ds.BackFace.StencilFunc = from_comparison(desc.stencil_back_face_func);
	return ds;
}

static D3D11_FILTER from_filter_type(const filter_type& type)
{
	switch (type)
	{
		default:
		case filter_type::point: return D3D11_FILTER_MIN_MAG_MIP_POINT;
		case filter_type::linear: return D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		case filter_type::anisotropic: return D3D11_FILTER_ANISOTROPIC;
		case filter_type::comparison_point: return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		case filter_type::comparison_linear: return D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		case filter_type::comparison_anisotropic: return D3D11_FILTER_COMPARISON_ANISOTROPIC;
	}
}

static D3D11_SAMPLER_DESC from_sampler_desc(const sampler_desc& desc)
{
	D3D11_SAMPLER_DESC sa;
	sa.Filter = from_filter_type(desc.filter);
	sa.AddressU = desc.clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
	sa.AddressV = desc.clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
	sa.AddressW = desc.clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
	sa.MipLODBias = desc.mip_bias;
	sa.MaxAnisotropy = desc.max_anisotropy;
	sa.ComparisonFunc = from_comparison(desc.comparison_func);
	for (int i = 0; i < 4; i++)
		sa.BorderColor[i] = desc.border_color[i];
	sa.MinLOD = desc.min_lod;
	sa.MaxLOD = desc.max_lod;
	return sa;
}

static resource_type to_resource_type(D3D11_RESOURCE_DIMENSION type)
{
	return (resource_type)type;
};

static D3D11_USAGE from_resource_usage(const resource_usage& usage)
{
	switch (usage)
	{
		default:
		case resource_usage::default: return D3D11_USAGE_DEFAULT;
		case resource_usage::upload: return D3D11_USAGE_DYNAMIC;
		case resource_usage::readback: return D3D11_USAGE_STAGING;
	}
}

static resource_usage to_resource_usage(D3D11_USAGE usage)
{
	switch (usage)
	{
		default:
		case D3D11_USAGE_DEFAULT:   return resource_usage::default;
		case D3D11_USAGE_IMMUTABLE: return resource_usage::immutable;
		case D3D11_USAGE_DYNAMIC:   return resource_usage::upload;
		case D3D11_USAGE_STAGING:   return resource_usage::readback;
	}
}

static uint16_t to_resource_bindings(uint32_t bind_flags, uint32_t misc_flags)
{
	uint16_t flags = 0;
	if (bind_flags & D3D11_BIND_RENDER_TARGET) flags |= resource_binding::render_target;
	if (bind_flags & D3D11_BIND_DEPTH_STENCIL) flags |= resource_binding::depth_stencil;
	if (bind_flags & D3D11_BIND_UNORDERED_ACCESS) flags |= resource_binding::unordered_access;
	if ((bind_flags & D3D11_BIND_SHADER_RESOURCE) == 0) flags |= resource_binding::not_shader_resource;
	if (misc_flags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) flags |= resource_binding::structured;
	if (misc_flags & D3D11_RESOURCE_MISC_TEXTURECUBE) flags |= resource_binding::texturecube;
	return flags;
}

static uint32_t access_flags(D3D11_USAGE usage)
{
	switch (usage)
	{
		case D3D11_USAGE_DYNAMIC: return D3D11_CPU_ACCESS_WRITE;
		case D3D11_USAGE_STAGING: return D3D11_CPU_ACCESS_READ;
		default: break;
	}

	return 0;
}

template<typename ResourceDescT>
static void make_staging(ResourceDescT* desc)
{
	desc->Usage = D3D11_USAGE_STAGING;
	desc->CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc->BindFlags = 0;
	desc->MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE; // keep cube flag because that's the only way to differentiate between 2D and cube types
}

static D3D11_RTV_DIMENSION from_rtv_dimension(const rtv_dimension& dim)
{
	return (D3D11_RTV_DIMENSION)dim;
}

static rtv_dimension to_rtv_dimension(const D3D11_RTV_DIMENSION& dim)
{
	return (rtv_dimension)dim;
}

static D3D11_RENDER_TARGET_VIEW_DESC from_rtv_desc(const rtv_desc& desc)
{
	D3D11_RENDER_TARGET_VIEW_DESC d;
	DXGI_FORMAT tf, df, sf;
	dxgi::get_compatible_formats(dxgi::from_surface_format(desc.format), &tf, &df, &sf);
	d.Format = tf;
	d.ViewDimension = from_rtv_dimension(desc.dimension);

	switch (desc.dimension)
	{
		case rtv_dimension::buffer:
			d.Buffer.FirstElement = desc.buffer.first_element;
			d.Buffer.NumElements = desc.buffer.num_elements;
			break;
		case rtv_dimension::texture1d:
			d.Texture1D.MipSlice = desc.texture1d.mip_slice;
			break;
		case rtv_dimension::texture1darray:
			d.Texture1DArray.MipSlice = desc.texture1darray.mip_slice;
			d.Texture1DArray.FirstArraySlice = desc.texture1darray.first_array_slice;
			d.Texture1DArray.ArraySize = desc.texture1darray.array_size;
			break;
		case rtv_dimension::texture2d:
			d.Texture2D.MipSlice = desc.texture2d.mip_slice;
			break;
		case rtv_dimension::texture2darray:
			d.Texture2DArray.MipSlice = desc.texture2darray.mip_slice;
			d.Texture2DArray.FirstArraySlice = desc.texture2darray.first_array_slice;
			d.Texture2DArray.ArraySize = desc.texture2darray.array_size;
			break;
		case rtv_dimension::texture2dms:
			break;
		case rtv_dimension::texture2dmsarray:
			d.Texture2DMSArray.FirstArraySlice = desc.texture2dmsarray.first_array_slice;
			d.Texture2DMSArray.ArraySize = desc.texture2dmsarray.array_size;
			break;
		case rtv_dimension::texture3d:
			d.Texture3D.MipSlice = desc.texture3d.mip_slice;
			d.Texture3D.FirstWSlice = desc.texture3d.first_wslice;
			d.Texture3D.WSize = desc.texture3d.wsize;
			break;
		
		default: oThrow(std::errc::invalid_argument, "invalid dimension %d", desc.dimension);
	}
	return d;
}

static rtv_desc to_rtv_desc(const D3D11_RENDER_TARGET_VIEW_DESC& desc)
{
	rtv_desc d;
	d.format = dxgi::to_surface_format(desc.Format);
	d.dimension = to_rtv_dimension(desc.ViewDimension);
	switch (desc.ViewDimension)
	{
		case D3D11_RTV_DIMENSION_BUFFER:
			d.buffer.first_element = desc.Buffer.FirstElement;
			d.buffer.num_elements = desc.Buffer.NumElements;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE1D:
			d.texture1d.mip_slice = desc.Texture1D.MipSlice;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE1DARRAY:
			d.texture1darray.mip_slice = desc.Texture1DArray.MipSlice;
			d.texture1darray.first_array_slice = desc.Texture1DArray.FirstArraySlice;
			d.texture1darray.array_size = desc.Texture1DArray.ArraySize;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE2D:
			d.texture2d.mip_slice = desc.Texture2D.MipSlice;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
			d.texture2darray.mip_slice = desc.Texture2DArray.MipSlice;
			d.texture2darray.first_array_slice = desc.Texture2DArray.FirstArraySlice;
			d.texture2darray.array_size = desc.Texture2DArray.ArraySize;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE2DMS:
			break;
		case D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY:
			d.texture2dmsarray.first_array_slice = desc.Texture2DMSArray.FirstArraySlice;
			d.texture2dmsarray.array_size = desc.Texture2DMSArray.ArraySize;
			break;
		case D3D11_RTV_DIMENSION_TEXTURE3D:
			d.texture3d.mip_slice = desc.Texture3D.MipSlice;
			d.texture3d.first_wslice = desc.Texture3D.FirstWSlice;
			d.texture3d.wsize = desc.Texture3D.WSize;
			break;
		
		default: oThrow(std::errc::invalid_argument, "invalid D3D11_RTV_DIMENSION %d", desc.ViewDimension);
	}
	return d;
}

static D3D11_DSV_DIMENSION from_dsv_dimension(const dsv_dimension& dim)
{
	return (D3D11_DSV_DIMENSION)dim;
}

static dsv_dimension to_dsv_dimension(const D3D11_DSV_DIMENSION& dim)
{
	return (dsv_dimension)dim;
}

static D3D11_DEPTH_STENCIL_VIEW_DESC from_dsv_desc(const dsv_desc& desc)
{
	D3D11_DEPTH_STENCIL_VIEW_DESC d;
	DXGI_FORMAT tf, df, sf;
	dxgi::get_compatible_formats(dxgi::from_surface_format(desc.format), &tf, &df, &sf);
	d.Format = df;
	d.ViewDimension = from_dsv_dimension(desc.dimension);
	d.Flags = 0;

	switch (desc.dimension)
	{
		case dsv_dimension::texture1d:
			d.Texture1D.MipSlice = desc.texture1d.mip_slice;
			break;
		case dsv_dimension::texture1darray:
			d.Texture1DArray.MipSlice = desc.texture1darray.mip_slice;
			d.Texture1DArray.FirstArraySlice = desc.texture1darray.first_array_slice;
			d.Texture1DArray.ArraySize = desc.texture1darray.array_size;
			break;
		case dsv_dimension::texture2d:
			d.Texture2D.MipSlice = desc.texture2d.mip_slice;
			break;
		case dsv_dimension::texture2darray:
			d.Texture2DArray.MipSlice = desc.texture2darray.mip_slice;
			d.Texture2DArray.FirstArraySlice = desc.texture2darray.first_array_slice;
			d.Texture2DArray.ArraySize = desc.texture2darray.array_size;
			break;
		case dsv_dimension::texture2dms:
			break;
		case dsv_dimension::texture2dmsarray:
			d.Texture2DMSArray.FirstArraySlice = desc.texture2dmsarray.first_array_slice;
			d.Texture2DMSArray.ArraySize = desc.texture2dmsarray.array_size;
			break;
		
		default: oThrow(std::errc::invalid_argument, "invalid dimension %d", desc.dimension);
	}
	return d;
}

static dsv_desc to_dsv_desc(const D3D11_DEPTH_STENCIL_VIEW_DESC& desc)
{
	dsv_desc d;
	d.format = dxgi::to_surface_format(desc.Format);
	d.dimension = to_dsv_dimension(desc.ViewDimension);
	d.dsv_flags = 0;

	switch (desc.ViewDimension)
	{
		case D3D11_DSV_DIMENSION_TEXTURE1D:
			d.texture1d.mip_slice = desc.Texture1D.MipSlice;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE1DARRAY:
			d.texture1darray.mip_slice = desc.Texture1DArray.MipSlice;
			d.texture1darray.first_array_slice = desc.Texture1DArray.FirstArraySlice;
			d.texture1darray.array_size = desc.Texture1DArray.ArraySize;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE2D:
			d.texture2d.mip_slice = desc.Texture2D.MipSlice;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
			d.texture2darray.mip_slice = desc.Texture2DArray.MipSlice;
			d.texture2darray.first_array_slice = desc.Texture2DArray.FirstArraySlice;
			d.texture2darray.array_size = desc.Texture2DArray.ArraySize;
			break;
		case D3D11_DSV_DIMENSION_TEXTURE2DMS:
			break;
		case D3D11_DSV_DIMENSION_TEXTURE2DMSARRAY:
			d.texture2dmsarray.first_array_slice = desc.Texture2DMSArray.FirstArraySlice;
			d.texture2dmsarray.array_size = desc.Texture2DMSArray.ArraySize;
			break;
		
		default: oThrow(std::errc::invalid_argument, "invalid D3D11_DSV_DIMENSION %d", desc.ViewDimension);
	}
	return d;
}

static D3D11_UAV_DIMENSION from_uav_dimension(const uav_dimension& dim)
{
	return (D3D11_UAV_DIMENSION)dim;
}

static uav_dimension to_uav_dimension(const D3D11_UAV_DIMENSION& dim)
{
	return (uav_dimension)dim;
}

static D3D11_UNORDERED_ACCESS_VIEW_DESC from_uav_desc(const uav_desc& desc)
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC d;
	d.Format = dxgi::from_surface_format(desc.format);
	d.ViewDimension = from_uav_dimension(desc.dimension);

	switch (desc.dimension)
	{
		case uav_dimension::buffer:
			d.Buffer.FirstElement = desc.buffer.first_element;
			d.Buffer.NumElements = desc.buffer.num_elements;
			switch (desc.buffer.extension)
			{
				default:
				case uav_extension::none:    d.Buffer.Flags = 0; break;
				case uav_extension::raw:     d.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_RAW; break;
				case uav_extension::append:  d.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND; break;
				case uav_extension::counter: d.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_COUNTER; break;
			}
			break;
		case uav_dimension::texture1d:
			d.Texture1D.MipSlice = desc.texture1d.mip_slice;
			break;
		case uav_dimension::texture1darray:
			d.Texture1DArray.MipSlice = desc.texture1darray.mip_slice;
			d.Texture1DArray.FirstArraySlice = desc.texture1darray.first_array_slice;
			d.Texture1DArray.ArraySize = desc.texture1darray.array_size;
			break;
		case uav_dimension::texture2d:
			d.Texture2D.MipSlice = desc.texture2d.mip_slice;
			break;
		case uav_dimension::texture2darray:
			d.Texture2DArray.MipSlice = desc.texture2darray.mip_slice;
			d.Texture2DArray.FirstArraySlice = desc.texture2darray.first_array_slice;
			d.Texture2DArray.ArraySize = desc.texture2darray.array_size;
			break;
		case uav_dimension::texture3d:
			d.Texture3D.MipSlice = desc.texture3d.mip_slice;
			d.Texture3D.FirstWSlice = desc.texture3d.first_wslice;
			d.Texture3D.WSize = desc.texture3d.wsize;
			break;
		
		default: oThrow(std::errc::invalid_argument, "invalid dimension %d", desc.dimension);
	}
	return d;
}

static uav_desc to_uav_desc(const D3D11_UNORDERED_ACCESS_VIEW_DESC& desc)
{
	uav_desc d;
	d.format = dxgi::to_surface_format(desc.Format);
	d.dimension = to_uav_dimension(desc.ViewDimension);

	switch (desc.ViewDimension)
	{
		case D3D11_UAV_DIMENSION_BUFFER:
			d.buffer.first_element = desc.Buffer.FirstElement;
			d.buffer.num_elements = desc.Buffer.NumElements;
			switch (desc.Buffer.Flags)
			{
				default:
				case 0:                             d.buffer.uav_extension::none; break;
				case D3D11_BUFFER_UAV_FLAG_RAW:     d.buffer.extension = uav_extension::raw; break;
				case D3D11_BUFFER_UAV_FLAG_APPEND:  d.buffer.extension = uav_extension::append; break;
				case D3D11_BUFFER_UAV_FLAG_COUNTER: d.buffer.extension = uav_extension::counter; break;
			}
			break;
		case D3D11_UAV_DIMENSION_TEXTURE1D:
			d.texture1d.mip_slice = desc.Texture1D.MipSlice;
			break;
		case D3D11_UAV_DIMENSION_TEXTURE1DARRAY:
			d.texture1darray.mip_slice = desc.Texture1DArray.MipSlice;
			d.texture1darray.first_array_slice = desc.Texture1DArray.FirstArraySlice;
			d.texture1darray.array_size = desc.Texture1DArray.ArraySize;
			break;
		case D3D11_UAV_DIMENSION_TEXTURE2D:
			d.texture2d.mip_slice = desc.Texture2D.MipSlice;
			break;
		case D3D11_UAV_DIMENSION_TEXTURE2DARRAY:
			d.texture2darray.mip_slice = desc.Texture2DArray.MipSlice;
			d.texture2darray.first_array_slice = desc.Texture2DArray.FirstArraySlice;
			d.texture2darray.array_size = desc.Texture2DArray.ArraySize;
			break;
		case D3D11_UAV_DIMENSION_TEXTURE3D:
			d.texture3d.mip_slice = desc.Texture3D.MipSlice;
			d.texture3d.first_wslice = desc.Texture3D.FirstWSlice;
			d.texture3d.wsize = desc.Texture3D.WSize;
			break;
		
		default: oThrow(std::errc::invalid_argument, "invalid D3D11_UAV_DIMENSION %d", desc.ViewDimension);
	}
	return d;
}

static D3D11_SRV_DIMENSION from_srv_dimension(const srv_dimension& dim)
{
	return (D3D11_SRV_DIMENSION)dim;
}

static srv_dimension to_srv_dimension(const D3D11_SRV_DIMENSION& dim)
{
	return (srv_dimension)dim;
}

static D3D11_SHADER_RESOURCE_VIEW_DESC from_srv_desc(const srv_desc& desc)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC d;
	DXGI_FORMAT tf, df, sf;
	dxgi::get_compatible_formats(dxgi::from_surface_format(desc.format), &tf, &df, &sf);
	d.Format = sf;
	d.Format = dxgi::from_surface_format(desc.format);
	d.ViewDimension = from_srv_dimension(desc.dimension);

	switch (desc.dimension)
	{
		case srv_dimension::buffer:
			d.Buffer.FirstElement = desc.buffer.first_element;
			d.Buffer.NumElements = desc.buffer.num_elements;
			break;
		case srv_dimension::texture1d:
			d.Texture1D.MostDetailedMip = desc.texture1d.most_detailed_mip;
			d.Texture1D.MipLevels = desc.texture1d.mip_levels;
			oGPU_CHECK(desc.texture1d.resource_min_lod_clamp == 0.0f, "resource_min_lod_clamp not supported");
			break;
		case srv_dimension::texture1darray:
			d.Texture1DArray.MostDetailedMip = desc.texture1darray.most_detailed_mip;
			d.Texture1DArray.MipLevels = desc.texture1darray.mip_levels;
			d.Texture1DArray.FirstArraySlice = desc.texture1darray.first_array_slice;
			d.Texture1DArray.ArraySize = desc.texture1darray.array_size;
			oGPU_CHECK(desc.texture1darray.resource_min_lod_clamp == 0.0f, "resource_min_lod_clamp not supported");
			break;
		case srv_dimension::texture2d:
			d.Texture2D.MostDetailedMip = desc.texture2d.most_detailed_mip;
			d.Texture2D.MipLevels = desc.texture2d.mip_levels;
			oGPU_CHECK(desc.texture2d.resource_min_lod_clamp == 0.0f, "resource_min_lod_clamp not supported");
			break;
		case srv_dimension::texture2darray:
			d.Texture2DArray.MostDetailedMip = desc.texture2darray.most_detailed_mip;
			d.Texture2DArray.MipLevels = desc.texture2darray.mip_levels;
			d.Texture2DArray.FirstArraySlice = desc.texture2darray.first_array_slice;
			d.Texture2DArray.ArraySize = desc.texture2darray.array_size;
			oGPU_CHECK(desc.texture2darray.resource_min_lod_clamp == 0.0f, "resource_min_lod_clamp not supported");
			break;
		case srv_dimension::texture3d:
			d.Texture3D.MostDetailedMip = desc.texture3d.most_detailed_mip;
			d.Texture3D.MipLevels = desc.texture3d.mip_levels;
			oGPU_CHECK(desc.texture3d.resource_min_lod_clamp == 0.0f, "resource_min_lod_clamp not supported");
			break;
		case srv_dimension::texturecube:
			d.TextureCube.MostDetailedMip = desc.texturecube.most_detailed_mip;
			d.TextureCube.MipLevels = desc.texturecube.mip_levels;
			oGPU_CHECK(desc.texturecube.resource_min_lod_clamp == 0.0f, "resource_min_lod_clamp not supported");
			break;
		case srv_dimension::texturecubearray:
			d.TextureCubeArray.MostDetailedMip = desc.texturecubearray.most_detailed_mip;
			d.TextureCubeArray.MipLevels = desc.texturecubearray.mip_levels;
			d.TextureCubeArray.First2DArrayFace = desc.texturecubearray.first_2d_array_face;
			d.TextureCubeArray.NumCubes = desc.texturecubearray.num_cubes;
			oGPU_CHECK(desc.texturecubearray.resource_min_lod_clamp == 0.0f, "resource_min_lod_clamp not supported");
			break;
		
		default: oThrow(std::errc::invalid_argument, "unexpected D3D11_RESOURCE_DIMENSION %d", desc.dimension);
	}
	return d;
}

static srv_desc to_srv_desc(const D3D11_SHADER_RESOURCE_VIEW_DESC& desc)
{
	srv_desc d;
	d.format = dxgi::to_surface_format(desc.Format);
	d.dimension = to_srv_dimension(desc.ViewDimension);

	switch (desc.ViewDimension)
	{
		case D3D11_SRV_DIMENSION_BUFFER:
			d.buffer.first_element = desc.Buffer.FirstElement;
			d.buffer.num_elements = desc.Buffer.NumElements;
			break;
		case D3D11_SRV_DIMENSION_TEXTURE1D:
			d.texture1d.most_detailed_mip = desc.Texture1D.MostDetailedMip;
			d.texture1d.mip_levels = desc.Texture1D.MipLevels;
			d.texture1d.resource_min_lod_clamp = 0.0f;
			break;
		case D3D11_SRV_DIMENSION_TEXTURE1DARRAY:
			d.texture1darray.most_detailed_mip = desc.Texture1DArray.MostDetailedMip;
			d.texture1darray.mip_levels = desc.Texture1DArray.MipLevels;
			d.texture1darray.first_array_slice = desc.Texture1DArray.FirstArraySlice;
			d.texture1darray.array_size = desc.Texture1DArray.ArraySize;
			d.texture1darray.resource_min_lod_clamp = 0.0f;
			break;
		case D3D11_SRV_DIMENSION_TEXTURE2D:
			d.texture2d.most_detailed_mip = desc.Texture2D.MostDetailedMip;
			d.texture2d.mip_levels = desc.Texture2D.MipLevels;
			d.texture2d.resource_min_lod_clamp = 0.0f;
			break;
		case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
			d.texture2darray.most_detailed_mip = desc.Texture2DArray.MostDetailedMip;
			d.texture2darray.mip_levels = desc.Texture2DArray.MipLevels;
			d.texture2darray.first_array_slice = desc.Texture2DArray.FirstArraySlice;
			d.texture2darray.array_size = desc.Texture2DArray.ArraySize;
			d.texture2darray.resource_min_lod_clamp = 0.0f;
			break;
		case D3D11_SRV_DIMENSION_TEXTURE3D:
			d.texture3d.most_detailed_mip = desc.Texture3D.MostDetailedMip;
			d.texture3d.mip_levels = desc.Texture3D.MipLevels;
			d.texture3d.resource_min_lod_clamp = 0.0f;
			break;
		case D3D11_SRV_DIMENSION_TEXTURECUBE:
			d.texturecube.most_detailed_mip = desc.TextureCube.MostDetailedMip;
			d.texturecube.mip_levels = desc.TextureCube.MipLevels;
			d.texturecube.resource_min_lod_clamp = 0.0f;
			break;
		case D3D11_SRV_DIMENSION_TEXTURECUBEARRAY:
			d.texturecubearray.most_detailed_mip = desc.TextureCubeArray.MostDetailedMip;
			d.texturecubearray.mip_levels = desc.TextureCubeArray.MipLevels;
			d.texturecubearray.first_2d_array_face = desc.TextureCubeArray.First2DArrayFace;
			d.texturecubearray.num_cubes = desc.TextureCubeArray.NumCubes;
			d.texturecubearray.resource_min_lod_clamp = 0.0f;
			break;
		
		default: oThrow(std::errc::invalid_argument, "unexpected D3D11_RESOURCE_DIMENSION %d", desc.ViewDimension);
	}
	return d;
}

static srv_dimension guess_srv_dimension(const resource_desc& desc)
{
	switch (desc.type)
	{
		default:
		case resource_type::buffer:    return srv_dimension::buffer;
		case resource_type::texture1d: return desc.depth_or_array_size > 1 ? srv_dimension::texture1darray : srv_dimension::texture1d;
		case resource_type::texture2d: return (desc.resource_bindings & resource_binding::texturecube)
                                            ? (desc.depth_or_array_size > 6 ? srv_dimension::texturecubearray : srv_dimension::texturecube)
                                            : (desc.depth_or_array_size > 1 ? srv_dimension::texture2darray   : srv_dimension::texture2d);
		case resource_type::texture3d: return srv_dimension::texture3d;
	}
}

static D3D11_SHADER_RESOURCE_VIEW_DESC srv_from_resource_desc(const resource_desc& desc)
{
	D3D11_SHADER_RESOURCE_VIEW_DESC d;
	DXGI_FORMAT tf, df, sf;
	dxgi::get_compatible_formats(dxgi::from_surface_format(desc.format), &tf, &df, &sf);
	d.Format = sf;
	auto desc_dimension = guess_srv_dimension(desc);
	d.ViewDimension = from_srv_dimension(desc_dimension);

	switch (desc_dimension)
	{
		case srv_dimension::buffer:
			d.Buffer.FirstElement = 0;
			d.Buffer.NumElements = desc.depth_or_array_size;
			break;
		case srv_dimension::texture1d:
			d.Texture1D.MostDetailedMip = 0;
			d.Texture1D.MipLevels = desc.mip_levels;
			break;
		case srv_dimension::texture1darray:
			d.Texture1DArray.MostDetailedMip = 0;
			d.Texture1DArray.MipLevels = desc.mip_levels;
			d.Texture1DArray.FirstArraySlice = 0;
			d.Texture1DArray.ArraySize = desc.depth_or_array_size;
			break;
		case srv_dimension::texture2d:
			d.Texture2D.MostDetailedMip = 0;
			d.Texture2D.MipLevels = desc.mip_levels;
			break;
		case srv_dimension::texture2darray:
			d.Texture2DArray.MostDetailedMip = 0;
			d.Texture2DArray.MipLevels = desc.mip_levels;
			d.Texture2DArray.FirstArraySlice = 0;
			d.Texture2DArray.ArraySize = desc.depth_or_array_size;
			break;
		case srv_dimension::texture3d:
			d.Texture3D.MostDetailedMip = 0;
			d.Texture3D.MipLevels = desc.mip_levels;
			break;
		case srv_dimension::texturecube:
			d.TextureCube.MostDetailedMip = 0;
			d.TextureCube.MipLevels = desc.mip_levels;
			break;
		case srv_dimension::texturecubearray:
			d.TextureCubeArray.MostDetailedMip = 0;
			d.TextureCubeArray.MipLevels = desc.mip_levels;
			d.TextureCubeArray.First2DArrayFace = 0;
			d.TextureCubeArray.NumCubes = desc.depth_or_array_size / 6;
			oGPU_CHECK((desc.depth_or_array_size % 6) == 0, "cubemap has unexpected number of slices");
			break;
		
		default: oThrow(std::errc::invalid_argument, "unexpected D3D11_RESOURCE_DIMENSION %d", desc_dimension);
	}
	return d;
}

static uint32_t from_clear_type(const clear_type& type)
{
	return (uint32_t)type + 1;
}

struct rso_d3d11
{
	std::array<ref<ID3D11SamplerState>, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT> samplers;
	std::array<fitted_cbuffer, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> cbuffers;
	std::atomic<int> refcount;
	device* dev;
};

struct pso_d3d11
{
	ref<ID3D11VertexShader> vs;
	ref<ID3D11HullShader> hs;
	ref<ID3D11DomainShader> ds;
	ref<ID3D11GeometryShader> gs;
	ref<ID3D11PixelShader> ps;
	ref<ID3D11InputLayout> ia;

	ref<ID3D11BlendState> blend_state;
	ref<ID3D11RasterizerState> rasterizer_state;
	ref<ID3D11DepthStencilState> depth_stencil_state;

	std::atomic<int> refcount;

	uint32_t sample_mask;
	uint32_t num_render_targets;
	surface::format rtv_formats[8];
	surface::format dsv_format;
	D3D11_PRIMITIVE_TOPOLOGY primitive_topology;

	uint32_t sample_count;
	uint32_t sample_quality;
	uint32_t node_mask;
	uint32_t flags;
	device* dev;
};

struct cso_d3d11
{
	ref<ID3D11ComputeShader> cs;
	uint32_t node_mask;
	uint32_t flags;
	std::atomic<int> refcount;
	device* dev;
};

static pso_d3d11 sNullPSO;
static cso_d3d11 sNullCSO;

struct timer_d3d11
{
	ref<ID3D11Query> start;
	ref<ID3D11Query> stop;
	ref<ID3D11Query> disjoint;
	std::atomic<int> refcount;
	device* dev;
};

struct graphics_command_list_d3d11
{
	ref<ID3D11DeviceContext> dc;
	ref<ID3DUserDefinedAnnotation> uda;
	bool supports_deferred_contexts;
	std::atomic<int> refcount;
	device* dev;
	resource* mesh_buffers[2];
	ref<rso>* rsos;
	ref<pso>* psos;
	ref<cso>* csos;
	uint16_t max_rsos;
	uint16_t max_psos;
	uint16_t max_csos;
	rso_d3d11* current_rso;
	pso_d3d11* current_pso;
	cso_d3d11* current_cso;
	float blend_factor[4];
	uint32_t stencil_ref;
	std::array<ID3D11Buffer*, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT> bound_cbuffers;
	uint32_t transient_offset; // only meaningful after a new_transient_* call and before its matching commit_*
	uint32_t num_transient_elements;
	uint32_t transient_stride_or_32bit;

	void reset()
	{
		current_rso = nullptr;
		current_pso = nullptr;
		current_cso = nullptr;
		blend_factor[3] = blend_factor[2] = blend_factor[1] = blend_factor[0] = 0.0f;
		stencil_ref = 0;
		bound_cbuffers.fill(nullptr);
		transient_offset = ~0u;
		num_transient_elements = 0;
		transient_stride_or_32bit = 0;
	}
};

const char* device_child::name(char* dst, size_t dst_size) const
{
	return debug_name(dst, dst_size, (const ID3D11DeviceChild*)this);
}

void rso::reference()
{
	auto rso = (rso_d3d11*)this;
	++rso->refcount;
}

void rso::release()
{
	auto rso = (rso_d3d11*)this;
	if (--rso->refcount == 0)
		rso->dev->del_rso(this);
}

void pso::reference()
{
	auto pso = (pso_d3d11*)this;
	++pso->refcount;
}

void pso::release()
{
	auto pso = (pso_d3d11*)this;
	if (--pso->refcount == 0)
		pso->dev->del_pso(this);
}

void cso::reference()
{
	auto cso = (cso_d3d11*)this;
	++cso->refcount;
}

void cso::release()
{
	auto cso = (cso_d3d11*)this;
	if (--cso->refcount == 0)
		cso->dev->del_cso(this);
}

void view::reference()
{
	((ID3D11View*)this)->AddRef();
}

void view::release()
{
	((ID3D11View*)this)->Release();
}

resource* view::get_resource() const
{
	ref<ID3D11Resource> r;
	((ID3D11View*)this)->GetResource(&r);
	return (resource*)r.c_ptr();
}

rtv_desc rtv::get_desc() const
{
	D3D11_RENDER_TARGET_VIEW_DESC desc;
	((ID3D11RenderTargetView*)this)->GetDesc(&desc);
	return to_rtv_desc(desc);
}

dsv_desc dsv::get_desc() const
{
	D3D11_DEPTH_STENCIL_VIEW_DESC desc;
	((ID3D11DepthStencilView*)this)->GetDesc(&desc);
	return to_dsv_desc(desc);
}

uav_desc uav::get_desc() const
{
	D3D11_UNORDERED_ACCESS_VIEW_DESC desc;
	((ID3D11UnorderedAccessView*)this)->GetDesc(&desc);
	return to_uav_desc(desc);
}

srv_desc srv::get_desc() const
{
	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	((ID3D11ShaderResourceView*)this)->GetDesc(&desc);
	return to_srv_desc(desc);
}

void resource::reference()
{
	((ID3D11Resource*)this)->AddRef();
}

void resource::release()
{
	((ID3D11Resource*)this)->Release();
}

bool resource::copy_to(void* dest, uint32_t row_pitch, uint32_t depth_pitch, bool flip, bool blocking) const
{
	ref<ID3D11Device> d3d;
	((ID3D11Resource*)this)->GetDevice(&d3d);

	ref<ID3D11DeviceContext> dc;
	d3d->GetImmediateContext(&dc);

	D3D11_MAPPED_SUBRESOURCE mapped;
	if (FAILED(dc->Map((ID3D11Resource*)this, 0, D3D11_MAP_READ, blocking ? 0 : D3D11_MAP_FLAG_DO_NOT_WAIT, &mapped)) && !mapped.pData)
		return false;

	D3D11_RESOURCE_DIMENSION type;
	((ID3D11Resource*)this)->GetType(&type);
	switch (type)
	{
		case D3D11_RESOURCE_DIMENSION_BUFFER:
		{
			D3D11_BUFFER_DESC desc;
			((ID3D11Buffer*)this)->GetDesc(&desc);
			memcpy(dest, mapped.pData, desc.ByteWidth);
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
		{
			D3D11_TEXTURE1D_DESC desc;
			((ID3D11Texture1D*)this)->GetDesc(&desc);
			memcpy(dest, mapped.pData, desc.Width * dxgi::get_size(desc.Format));
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
		{
			D3D11_TEXTURE2D_DESC desc;
			((ID3D11Texture2D*)this)->GetDesc(&desc);
			memcpy2d(dest, row_pitch, mapped.pData, mapped.RowPitch, desc.Width * dxgi::get_size(desc.Format), desc.Height, flip);
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
		{
			D3D11_TEXTURE3D_DESC desc;
			((ID3D11Texture3D*)this)->GetDesc(&desc);
			memcpy3d(dest, row_pitch, depth_pitch, mapped.pData, mapped.RowPitch, mapped.DepthPitch, desc.Width * dxgi::get_size(desc.Format), desc.Height, desc.Depth);
			break;
		}
	}

	dc->Unmap((ID3D11Buffer*)this, 0);
	return true;
}

resource_desc resource::get_desc() const
{
	resource_desc d;

	D3D11_RESOURCE_DIMENSION type;
	((ID3D11Resource*)this)->GetType(&type);
	switch (type)
	{
		case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
		{
			D3D11_TEXTURE1D_DESC desc;
			((ID3D11Texture1D*)this)->GetDesc(&desc);
			d.width = desc.Width;
			d.height = 1;
			d.depth_or_array_size = (uint16_t)desc.ArraySize;
			d.mip_levels = (uint16_t)desc.MipLevels;
			d.alignment = 0;
			d.sample_count = 1;
			d.sample_quality = 0;
			d.type = to_resource_type(type);
			d.usage = to_resource_usage(desc.Usage);
			d.format = dxgi::to_surface_format(desc.Format);
			d.layout = texture_layout::standard_swizzle;
			d.resource_bindings = to_resource_bindings(desc.BindFlags, desc.MiscFlags);
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
		{
			D3D11_TEXTURE2D_DESC desc;
			((ID3D11Texture2D*)this)->GetDesc(&desc);
			d.width = desc.Width;
			d.height = desc.Height;
			d.depth_or_array_size = (uint16_t)desc.ArraySize;
			d.mip_levels = (uint16_t)desc.MipLevels;
			d.alignment = 0;
			d.sample_count = (uint8_t)desc.SampleDesc.Count;
			d.sample_quality = (uint8_t)desc.SampleDesc.Quality;
			d.type = to_resource_type(type);
			d.usage = to_resource_usage(desc.Usage);
			d.format = dxgi::to_surface_format(desc.Format);
			d.layout = texture_layout::standard_swizzle;
			d.resource_bindings = to_resource_bindings(desc.BindFlags, desc.MiscFlags);
			break;
		}

		case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
		{
			D3D11_TEXTURE3D_DESC desc;
			((ID3D11Texture3D*)this)->GetDesc(&desc);
			d.width = desc.Width;
			d.height = desc.Height;
			d.depth_or_array_size = (uint16_t)desc.Depth;
			d.mip_levels = (uint16_t)desc.MipLevels;
			d.alignment = 0;
			d.sample_count = 1;
			d.sample_quality = 0;
			d.type = to_resource_type(type);
			d.usage = to_resource_usage(desc.Usage);
			d.format = dxgi::to_surface_format(desc.Format);
			d.layout = texture_layout::standard_swizzle;
			d.resource_bindings = to_resource_bindings(desc.BindFlags, desc.MiscFlags);
			break;
		}

		case D3D11_RESOURCE_DIMENSION_BUFFER:
		{
			D3D11_BUFFER_DESC desc;
			((ID3D11Buffer*)this)->GetDesc(&desc);
			d.width = desc.ByteWidth;
			d.height = 1;
			d.depth_or_array_size = (uint16_t)(desc.ByteWidth / max(1u, desc.StructureByteStride));
			d.mip_levels = 0;
			d.alignment = 0;
			d.sample_count = 1;
			d.sample_quality = 0;
			d.type = to_resource_type(type);
			d.usage = to_resource_usage(desc.Usage);
			d.format = surface::format::unknown;
			d.layout = texture_layout::unknown;
			d.resource_bindings = to_resource_bindings(desc.BindFlags, desc.MiscFlags);
			break;
		};

		default: oThrow(std::errc::invalid_argument, "unexpected D3D11_RESOURCE_DIMENSION %d", type);
	}

	return d;
}

surface::image resource::make_snapshot(const allocator& a)
{
	D3D11_RESOURCE_DIMENSION type;
	((ID3D11Resource*)this)->GetType(&type);
	oGPU_CHECK(type == D3D11_RESOURCE_DIMENSION_TEXTURE2D, "make_snapshot supports only texture2d resources");
	return d3d::make_snapshot((ID3D11Texture2D*)this, false, a);
}

double timer::get_time(bool blocking)
{
	auto tm = (timer_d3d11*)this;

	ref<ID3D11Device> d3d;
	tm->start->GetDevice(&d3d);

	uint64_t start_results, stop_results;
	
	if (!copy_async_data(d3d, tm->start, &start_results, blocking))
		return -1.0;

	if (!copy_async_data(d3d, tm->stop, &stop_results, blocking))
		return -1.0;

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint_data;
	if (!copy_async_data(d3d, tm->disjoint, &disjoint_data, blocking))
		return -1.0;

	oGPU_CHECK(!disjoint_data.Disjoint, "timer is disjoint (laptop AC unplugged, overheating, GPU throttling)");
	return (stop_results - start_results) / double(disjoint_data.Frequency);
}

void timer::reference()
{
	auto t = (timer_d3d11*)this;
	++t->refcount;
}

void timer::release()
{
	auto t = (timer_d3d11*)this;
	if (--t->refcount == 0)
		t->dev->del_timer(this);
}

void fence::reference()
{
	((ID3D11Query*)this)->AddRef();
}

void fence::release()
{
	((ID3D11Query*)this)->Release();
}

void occlusion_query::reference()
{
	((ID3D11Query*)this)->AddRef();
}

void occlusion_query::release()
{
	((ID3D11Query*)this)->Release();
}

void stats_query::reference()
{
	((ID3D11Query*)this)->AddRef();
}

void stats_query::release()
{
	((ID3D11Query*)this)->Release();
}

uint32_t occlusion_query::get_pixel_count(bool blocking)
{
	auto q = (ID3D11Query*)this;

	ref<ID3D11Device> d3d;
	q->GetDevice(&d3d);

	ref<ID3D11DeviceContext> dc;
	d3d->GetImmediateContext(&dc);

	uint32_t pixel_count = 0xffffffff;
	copy_async_data(d3d, q, &pixel_count, sizeof(pixel_count), blocking);
	return 0xffffffff;
}

bool fence::finished(bool blocking)
{
	auto q = (ID3D11Query*)this;

	ref<ID3D11Device> d3d;
	q->GetDevice(&d3d);

	BOOL is_finished = FALSE;
	copy_async_data(d3d, q, &is_finished, sizeof(is_finished), blocking);
	return !!is_finished;
}

stats_desc stats_query::get_stats(bool blocking)
{
	auto q = (ID3D11Query*)this;

	ref<ID3D11Device> d3d;
	q->GetDevice(&d3d);

	stats_desc out_stats;
	D3D11_QUERY_DATA_PIPELINE_STATISTICS stats;
	if (copy_async_data(d3d, q, &stats, sizeof(stats), blocking))
	{
		out_stats.num_input_vertices = (uint32_t)stats.IAVertices;
		out_stats.num_input_primitives = (uint32_t)stats.IAPrimitives;
		out_stats.num_gs_output_primitives = (uint32_t)stats.GSPrimitives;
		out_stats.num_pre_clip_rasterizer_primitives = (uint32_t)stats.CInvocations;
		out_stats.num_post_clip_rasterizer_primitives = (uint32_t)stats.CPrimitives;
		out_stats.num_vs_calls = (uint32_t)stats.VSInvocations;
		out_stats.num_hs_calls = (uint32_t)stats.HSInvocations;
		out_stats.num_ds_calls = (uint32_t)stats.DSInvocations;
		out_stats.num_gs_calls = (uint32_t)stats.GSInvocations;
		out_stats.num_ps_calls = (uint32_t)stats.PSInvocations;
		out_stats.num_cs_calls = (uint32_t)stats.CSInvocations;
	}
	else
		memset(&out_stats, 0xff, sizeof(out_stats));

	return out_stats;
}

device::device(const device_init& init, window* win)
	: alloc_(default_allocator)
	, temp_alloc_(default_allocator)
	, win_(win)
	, dev_(nullptr)
	, imm_(nullptr)
	, flush_sync_(nullptr)
	, swp_(nullptr)
	, swp_srv_(nullptr)
	, swp_rtv_(nullptr)
	, swp_uav_(nullptr)
	, psos_(nullptr)
	, csos_(nullptr)
	, init_(init)
	, refcount_(1)
	, npresents_(0)
	, supports_deferred_(false)
	, transient_ring_start_(0)
	, transient_ring_end_(0)
	, transient_fence_index_(0)
{
	next_transient_ring_ends_.fill(0);

	ref<ID3D11Device> d3d = make_device(init);
	d3d->AddRef();
	dev_ = d3d;

	ref<ID3D11DeviceContext> idc;
	d3d->GetImmediateContext(&idc);

	supports_deferred_ = supports_deferred_contexts(d3d);

	if (win_)
	{
		oGPU_CHECK(!win->render_target(), "The specified window is already associated with a render target and cannot be reassociated.");
		window_shape s = win->shape();
		oGPU_CHECK(!has_statusbar(s.style), "A window used for rendering must not have a status bar");

		auto& win_init = win->init();
		oGPU_CHECK(all(win_init.min_client_size > int2(0, 0)), "A window's minimum dimensions must be non-zero to protect against minimizing the window causing degenerate targets. It's recommended the size work well with other render targets so special handling isn't required for minimized windows.");

		ref<IDXGISwapChain> sc = dxgi::make_swap_chain(d3d
			, false
			, s.client_size.x
			, s.client_size.y
			, false
			, surface::format::b8g8r8a8_unorm
			, 0
			, 0
			, (HWND)win->native_handle()
			, false);

		win->render_target(true);
		sc->AddRef();
		swp_ = sc;

		DXGI_SWAP_CHAIN_DESC desc;
		sc->GetDesc(&desc);
		resize(desc.BufferDesc.Width, desc.BufferDesc.Height);
	}

	const size_t rso_bytes = sizeof(pso*) * init.max_rsos;
	const size_t pso_bytes = sizeof(pso*) * init.max_psos;
	const size_t cso_bytes = sizeof(cso*) * init.max_csos;
	const size_t bookkeeping_bytes = rso_bytes + pso_bytes + cso_bytes;
	void* mem = default_allocate(bookkeeping_bytes, "graphics objects by index");
	memset(mem, 0, bookkeeping_bytes);

	rsos_ = (ref<rso>*)mem;
	psos_ = (ref<pso>*)(rsos_ + init.max_rsos);
	csos_ = (ref<cso>*)(psos_ + init.max_psos);

	flush_sync_ = new_fence("flush_sync");
	
	// persistent mesh heap
	{
		auto common_alignment = memory_alignment::align64k; // both gpu and cpu should share the same alignment

		allocate_options opts(memory_type::cpu_writeback, common_alignment);

		const uint32_t capacity_bytes = init.max_persistent_mesh_bytes;

		const size_t persistent_bookkeeping_bytes = opts.align(persistent_mesh_alloc_.calc_bookkeeping_size(capacity_bytes));
		const size_t cpu_bytes = persistent_bookkeeping_bytes + capacity_bytes;

		void* cpu_mem = alloc_(cpu_bytes, "device persistent mesh bookkeeping", opts);

		void* bookkeeping = cpu_mem;
		persistent_mesh_alloc_.initialize((uint8_t*)bookkeeping + persistent_bookkeeping_bytes, capacity_bytes, bookkeeping);

		resource_desc desc;
		desc.width = capacity_bytes;
		desc.height = 1;
		desc.depth_or_array_size = 1;
		desc.mip_levels = 1;
		desc.alignment = (uint16_t)opts.alignment_bytes();
		desc.sample_count = 1;
		desc.sample_quality = 0;
		desc.type = resource_type::buffer;
		desc.usage = resource_usage::default;
		desc.format = surface::format::unknown;
		desc.layout = texture_layout::unknown;
		desc.resource_bindings = 0;

		persistent_mesh_buffer_ = new_resource("device persistent mesh buffer", desc);
	}

	// transient mesh heap
	{
		resource_desc desc;
		desc.width = init.max_transient_mesh_bytes;
		desc.height = 1;
		desc.depth_or_array_size = 1;
		desc.mip_levels = 1;
		desc.alignment = sizeof(uint32_t);
		desc.sample_count = 1;
		desc.sample_quality = 0;
		desc.type = resource_type::buffer;
		desc.usage = resource_usage::upload;
		desc.format = surface::format::unknown;
		desc.layout = texture_layout::unknown;
		desc.resource_bindings = 0;

		transient_mesh_buffer_ = new_resource("device transient mesh buffer", desc);
		transient_ring_start_ = 0;
		transient_ring_end_ = desc.width;

		for (auto& f : transient_fences_)
			f = new_fence("transient_vbuffer");
	}

	imm_ = new_graphics_command_list_internal("immediate graphics command list", false);
}

void device::reference()
{
	++refcount_;
}

void device::release()
{
	if (dev_ && --refcount_ == 0)
	{
		persistent_mesh_alloc_.reset(); // don't complain about allocations since the whole arena is about to be freed
		alloc_.deallocate(persistent_mesh_alloc_.deinitialize());

		persistent_mesh_buffer_ = nullptr;

		// reset transient mesh buffer
		{
			transient_mesh_buffer_ = nullptr;

			transient_ring_start_ = 0;
			transient_ring_end_ = 0;
			transient_fence_index_ = 0;

			transient_fences_.fill(nullptr);
			next_transient_ring_ends_.fill(0);
		}

		imm_ = nullptr;
		flush_sync_ = nullptr;

		for (uint16_t i = 0; i < init_.max_rsos; i++)
			if (rsos_[i])
				rsos_[i] = nullptr;

		for (uint16_t i = 0; i < init_.max_psos; i++)
			if (psos_[i])
				psos_[i] = nullptr;

		for (uint16_t i = 0; i < init_.max_csos; i++)
			if (csos_[i])
				csos_[i] = nullptr;

		default_deallocate(rsos_);
		psos_ = nullptr;
		csos_ = nullptr;
		resize(0, 0);
		oSAFE_RELEASEV(swp_);
		oSAFE_RELEASEV(dev_);
		win_ = nullptr;
		supports_deferred_ = false;

		this->~device();
		default_deallocate(this);
	}
}

ref<device> new_device(const device_init& init, window* win)
{
	allocate_options opts(memory_alignment::align64);

	void* mem = default_allocate(sizeof(device), "gpu device", opts);
	device* dev = new (mem) device(init, win);

	return ref<device>(dev, false);
}

void device::on_window_resizing()
{
	// do a simple clear. Event clearing and presenting the primary target seems
	// to create a case where the device can be lost if any of its state is 
	// changed. This is probably worth revisiting, but KISS for now.

	if (win_)
	{
		HWND hwnd = (HWND)win_->native_handle();
		windows::gdi::scoped_getdc hdc(hwnd);
		RECT r;
		GetClientRect(hwnd, &r);
		windows::gdi::scoped_brush brush(windows::gdi::make_brush(color::almost_black));
		FillRect(hdc, &r, brush);
	}
}

void device::on_window_resized()
{
	HWND hwnd = (HWND)win_->native_handle();
	RECT r;
	GetClientRect(hwnd, &r);
	resize(int(r.right - r.left), int(r.bottom - r.top));
}

void device::resize(uint32_t width, uint32_t height)
{
	if (!swp_)
		return;
	
	oGPU_CHECK(dev_, "");
	auto d3d = (ID3D11Device*)dev_;

	if (width == 0 || height == 0)
	{
		swp_srv_ = nullptr;
		swp_rtv_ = nullptr;
		swp_uav_ = nullptr;
		return;
	}

	uint32_t new_width = width;
	uint32_t new_height = height;
	{
		BOOL is_fullscreen = FALSE;
		ref<IDXGIOutput> output;
		((IDXGISwapChain*)swp_)->GetFullscreenState(&is_fullscreen, &output);
		if (is_fullscreen)
		{
			DXGI_OUTPUT_DESC desc;
			output->GetDesc(&desc);
			new_width = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
			new_height = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;
		}
	}

	uint32_t current_width = 0;
	uint32_t current_height = 0;
	if (swp_rtv_)
	{
		auto desc = swp_rtv_->get_resource()->get_desc();
		current_width = desc.width;
		current_height = desc.height;
	}

	if (current_width != new_width || current_height != new_height || !swp_rtv_)
	{
		#if oHAS_oTRACEA
			char target_name[128];
		#endif
		oTraceA("%s %s resize %dx%d -> %dx%d", type_name(typeid(*this).name()), debug_name(target_name, (ID3D11Device*)dev_), current_width, current_height, new_width, new_height);

		swp_srv_ = nullptr;
		swp_rtv_ = nullptr;
		swp_uav_ = nullptr;
		
		ref<ID3D11DeviceContext> dc;
		((ID3D11Device*)dev_)->GetImmediateContext(&dc);

		// ensure all render targets are clear to ensure the swapchain as a render target isn't bound
		{
			ID3D11UnorderedAccessView* null_uavs[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
			memset(null_uavs, 0, sizeof(null_uavs));
			dc->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, null_uavs, nullptr);
			dc->CSSetUnorderedAccessViews(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, null_uavs, nullptr);
		}

		auto sc = (IDXGISwapChain*)swp_;
		dxgi::resize_buffers(sc, int2(new_width, new_height));

		// rebuild derived resources
		ref<ID3D11Texture2D> swp_tex;
		oV(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&swp_tex));
		debug_name(swp_tex, "primary target");
				
		ref<ID3D11RenderTargetView> new_target;
		oV(d3d->CreateRenderTargetView(swp_tex, nullptr, &new_target));
		
		ref<ID3D11ShaderResourceView> new_resource;
		oV(d3d->CreateShaderResourceView(swp_tex, nullptr, &new_resource));

		D3D11_TEXTURE2D_DESC desc;
		swp_tex->GetDesc(&desc);
		if (desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS)
		{
			ref<ID3D11UnorderedAccessView> new_uav;
			oV(d3d->CreateUnorderedAccessView(swp_tex, nullptr, &new_uav));
			swp_uav_ = (uav*)new_uav.c_ptr();
		}

		swp_rtv_ = (rtv*)new_target.c_ptr();
		swp_srv_ = (srv*)new_resource.c_ptr();
	}
}

present_mode device::get_present_mode()
{
	if (!swp_)
		return present_mode::headless;
	BOOL full_screen = FALSE;
	((IDXGISwapChain*)swp_)->GetFullscreenState(&full_screen, nullptr);
	if (full_screen)
		return present_mode::fullscreen_exclusive;
	auto shape = win_->shape();
	return shape.state == window_state::fullscreen ? present_mode::fullscreen_cooperative : present_mode::windowed;
}

void device::set_present_mode(const present_mode& mode)
{
	oCheck(swp_, std::errc::protocol_error, "no primary render target has been created");
	
	BOOL was_excl = FALSE;
	((IDXGISwapChain*)swp_)->GetFullscreenState(&was_excl, nullptr);

	if (was_excl && mode != present_mode::fullscreen_exclusive)
		dxgi::set_fullscreen_exclusive((IDXGISwapChain*)swp_, false);

	if (win_)
	{
		switch (mode)
		{
			case present_mode::headless:               win_->hide();                          break;
			case present_mode::windowed:               win_->restore();                       break;
			case present_mode::fullscreen_cooperative: win_->state(window_state::fullscreen); break;
			case present_mode::fullscreen_exclusive:   win_->restore();                       break;
		}
	}

	if (!was_excl && mode == present_mode::fullscreen_exclusive)
	{
		oGPU_CHECK(!win_->parent(), "child windows cannot go fullscreen exclusive, remove the parent-child relationship first before calling set_present_mode");
		dxgi::set_fullscreen_exclusive((IDXGISwapChain*)swp_, true);
	}
}

void device::present(uint32_t interval)
{
	oCheck(swp_, std::errc::protocol_error, "no primary render target has been created");
	dxgi::present((IDXGISwapChain*)swp_, interval);
	npresents_++;
}

device_desc device::get_desc() const
{
	return d3d::get_desc((ID3D11Device*)dev_, init_.use_software_emulation);
}

void device::begin_frame()
{
	resolve_transient_fences();
}

void device::end_frame()
{
	insert_transient_fence();
}

void device::reset()
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->ClearState();
}

void device::execute(uint32_t num_command_lists, graphics_command_list* const* command_lists)
{
	auto imm = ((graphics_command_list_d3d11*)imm_.c_ptr())->dc;
	for (uint32_t i = 0; i < num_command_lists; i++)
	{
		ref<ID3D11CommandList> cl;
		graphics_command_list_d3d11* gcl = (graphics_command_list_d3d11*)command_lists[i];
		auto dc = gcl->dc;
		oV(dc->FinishCommandList(false, &cl));
		imm->ExecuteCommandList(cl, false);
		gcl->reset();
	}
}

void device::flush(const flush_type& type)
{
	auto dc = ((graphics_command_list_d3d11*)imm_.c_ptr())->dc;

	oAssert(type != flush_type::periodic, "periodic is not yet implemented");

	if (type == flush_type::immediate_and_sync)
		imm_->insert_fence(flush_sync_);

	dc->Flush();

	if (type == flush_type::immediate_and_sync)
	{
		backoff bo;
		while (!flush_sync_->finished())
			bo.pause();
	}
}

ref<rso> device::new_rso(const char* name, const root_signature_desc& desc)
{
	auto d3d = (ID3D11Device*)dev_;
	rso_d3d11* o = new (default_allocate(sizeof(rso_d3d11), "rso_d3d11")) rso_d3d11();

	for (uint32_t i = 0; i < desc.num_samplers; i++)
	{
		auto sd = from_sampler_desc(desc.samplers[i]);
		oV(d3d->CreateSamplerState(&sd, &o->samplers[i]));
		debug_name(o->samplers[i], name);
	}

	oGPU_CHECK(desc.num_cbvs < o->cbuffers.size(), "num_cbvs must be <= %u", o->cbuffers.size());
	size_t len = strlen(name) + 1 + 5; // add some room for numbers
	char* buf = (char*)alloca(len);
	int offset = snprintf(buf, len, "%s", name);
	char* num = buf + offset;
	size_t num_len = len - offset;

	for (uint32_t i = 0; i < desc.num_cbvs; i++)
	{
		snprintf(num, num_len, "%s%02u", name, i);
		o->cbuffers[i].initialize(buf, d3d, align(desc.struct_strides[i], 16), desc.max_num_structs[i]);
	}

	o->dev = this;
	o->refcount = 1;
	reference();
	return ref<rso>((rso*)o, false);
}

void device::new_rso(const char* name, const root_signature_desc& desc, uint32_t rso_index)
{
	char tmp[128];
	oGPU_CHECK(rso_index < init_.max_rsos, "rso_index larger than the max %u specified at device init time", init_.max_rsos);
	oGPU_CHECK(!rsos_[rso_index], "rso %s already created at index %u", rsos_[rso_index]->name(tmp), rso_index);
	rsos_[rso_index] = new_rso(name, desc);
	release();
}

void device::del_rso(rso* o)
{
	if (o)
	{
		auto r = (rso_d3d11*)o;
		r->samplers.fill(nullptr);
		r->~rso_d3d11();
		default_deallocate(r);
		release();
	}
}

ref<pso> device::new_pso(const char* name, const pipeline_state_desc& desc)
{
	// Challenge: These Creates can hash to the same object - common since 
	// psos share much of the same state. So how to name these uniquely?
	// probably can't.

	auto d3d = (ID3D11Device*)dev_;
	auto o = (pso_d3d11*)default_allocate(sizeof(pso_d3d11), "pso_d3d11");
	memset(o, 0, sizeof(pso_d3d11));

	if (desc.vs_bytecode) { oV(d3d->CreateVertexShader  (desc.vs_bytecode, bytecode_size(desc.vs_bytecode), nullptr, &o->vs)); debug_name(o->vs, "pso.vs"); }
	if (desc.hs_bytecode) { oV(d3d->CreateHullShader    (desc.hs_bytecode, bytecode_size(desc.hs_bytecode), nullptr, &o->hs)); debug_name(o->hs, "pso.hs"); }
	if (desc.ds_bytecode) { oV(d3d->CreateDomainShader  (desc.ds_bytecode, bytecode_size(desc.ds_bytecode), nullptr, &o->ds)); debug_name(o->ds, "pso.ds"); }
	if (desc.gs_bytecode) { oV(d3d->CreateGeometryShader(desc.gs_bytecode, bytecode_size(desc.gs_bytecode), nullptr, &o->gs)); debug_name(o->gs, "pso.gs"); }
	if (desc.ps_bytecode) { oV(d3d->CreatePixelShader   (desc.ps_bytecode, bytecode_size(desc.ps_bytecode), nullptr, &o->ps)); debug_name(o->ps, "pso.ps"); }

	if (desc.input_elements && desc.vs_bytecode)
	{
		D3D11_INPUT_ELEMENT_DESC element_descs[D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT];
		uint32_t n = 0;
		const uint32_t count = min(desc.num_input_elements, (uint32_t)D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT);
		for (uint32_t i = 0; i < count; i++)
		{
			const mesh::element_t& e = desc.input_elements[i];
			if (e.semantic == mesh::element_semantic::unknown || e.format == surface::format::unknown)
				continue;
			element_descs[n++] = from_input_element_desc(e);
		}
		oV(d3d->CreateInputLayout(element_descs, n, desc.vs_bytecode, bytecode_size(desc.vs_bytecode), &o->ia));
		debug_name(o->ia, "pso.ia");
	}

	{
		auto rs_desc = from_rasterizer_desc(desc.rasterizer_state);
		oV(d3d->CreateRasterizerState(&rs_desc, &o->rasterizer_state));
		debug_name(o->rasterizer_state, "pso.rs");

		auto bs_desc = from_blend_desc(desc.blend_state);
		oV(d3d->CreateBlendState(&bs_desc, &o->blend_state));
		debug_name(o->blend_state, "pso.bs");

		auto ds_desc = from_depth_stencil_desc(desc.depth_stencil_state);
		oV(d3d->CreateDepthStencilState(&ds_desc, &o->depth_stencil_state));
		debug_name(o->depth_stencil_state, "pso.dss");
	}

	o->sample_mask = basic::default_sample_mask;//desc.sample_mask;
	o->num_render_targets = desc.num_render_targets;
	for (uint32_t i = 0; i < 8; i++)
		o->rtv_formats[i] = desc.rtv_formats[i];
	o->dsv_format = desc.dsv_format;
	o->primitive_topology = from_primitive_type(desc.primitive_type);
	o->sample_count = desc.sample_quality;
	o->sample_quality = desc.sample_count;
	o->node_mask = desc.node_mask;
	o->flags = desc.flags;
	o->dev = this;
	o->refcount = 1;
	reference();
	return ref<pso>((pso*)o, false);
}

void device::new_pso(const char* name, const pipeline_state_desc& desc, uint32_t pso_index)
{
	char tmp[128];
	oGPU_CHECK(pso_index < init_.max_psos, "pso_index larger than the max %u specified at device init time", init_.max_psos);
	oGPU_CHECK(!psos_[pso_index], "pso %s already created at index %u", psos_[pso_index]->name(tmp), pso_index);
	psos_[pso_index] = new_pso(name, desc);
	oTrace("[GPU] PSO%03d = %s", pso_index, name);
	release();
}

void device::del_pso(pso* o)
{
	if (o)
	{
		for (uint16_t i = 0; i < init_.max_psos; i++)
			if (o == psos_[i])
				psos_[i] = nullptr;

		((pso_d3d11*)o)->~pso_d3d11();
		default_deallocate(o);
		release();
	}
}

ref<cso> device::new_cso(const char* name, const compute_state_desc& desc)
{
	auto d3d = (ID3D11Device*)dev_;
	auto o = (cso_d3d11*)default_allocate(sizeof(cso_d3d11), "cso_d3d11");
	memset(o, 0, sizeof(cso_d3d11));
	if (desc.cs_bytecode) { oV(d3d->CreateComputeShader(desc.cs_bytecode, bytecode_size(desc.cs_bytecode), nullptr, &o->cs)); debug_name(o->cs, name); }
	o->node_mask = desc.node_mask;
	o->flags = desc.flags;
	o->dev = this;
	o->refcount = 1;
	reference();
	return ref<cso>((cso*)o, false);
}

void device::new_cso(const char* name, const compute_state_desc& desc, uint32_t cso_index)
{
	char tmp[128];
	oGPU_CHECK(cso_index < init_.max_csos, "cso_index larger than the max %u specified at device init time", init_.max_csos);
	oGPU_CHECK(!csos_[cso_index], "cso %s already created at index %u", csos_[cso_index]->name(tmp), cso_index);
	csos_[cso_index] = new_cso(name, desc);
	release();
}

void device::del_cso(cso* o)
{
	if (o)
	{
		for (uint16_t i = 0; i < init_.max_csos; i++)
			if (o == csos_[i])
				csos_[i] = nullptr;

		((cso_d3d11*)o)->~cso_d3d11();
		default_deallocate(o);
		release();
	}
}

ref<graphics_command_list> device::new_graphics_command_list(const char* name)
{
	return new_graphics_command_list_internal(name, true);
}

ref<graphics_command_list> device::new_graphics_command_list_internal(const char* name, bool deferred)
{
	auto d3d = (ID3D11Device*)dev_;
	auto gcl = (graphics_command_list_d3d11*)default_allocate(sizeof(graphics_command_list_d3d11), "graphics command list d3d11");
	memset(gcl, 0, sizeof(graphics_command_list_d3d11));
	
	if (deferred)
	{
		HRESULT hr = d3d->CreateDeferredContext(0, &gcl->dc);
		if (FAILED(hr))
		{
			default_deallocate(gcl);

			char err[128];
			uint32_t new_flags = d3d->GetCreationFlags();
			if (new_flags & D3D11_CREATE_DEVICE_SINGLETHREADED)
				snprintf(err, "command_lists cannot be created on a device created as single-threaded.");
			else
				snprintf(err, "failed to create command_list %s: ", name);
			throw windows::error(hr, err);
		}
	}

	else
		d3d->GetImmediateContext(&gcl->dc);

	debug_name(gcl->dc, name);

	gcl->dev = this;

	if (FAILED(gcl->dc->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&gcl->uda)))
		oTrace("ID3DUserDefinedAnnotation creation failed, there will be no debug markup for this session");

	gcl->refcount = 1;
	gcl->mesh_buffers[0] = persistent_mesh_buffer_;
	gcl->mesh_buffers[1] = transient_mesh_buffer_;
	gcl->rsos = rsos_;
	gcl->psos = psos_;
	gcl->csos = csos_;
	gcl->max_rsos = init_.max_rsos;
	gcl->max_psos = init_.max_psos;
	gcl->max_csos = init_.max_csos;
	gcl->supports_deferred_contexts = supports_deferred_contexts(d3d);

	if (D3D11_DEVICE_CONTEXT_DEFERRED == gcl->dc->GetType())
		reference();
	return ref<graphics_command_list>((graphics_command_list*)gcl, false);
}

void device::del_graphics_command_list(graphics_command_list* cl)
{
	if (cl)
	{
		auto gcl = (graphics_command_list_d3d11*)cl;
		bool is_deferred = D3D11_DEVICE_CONTEXT_DEFERRED == gcl->dc->GetType();
		gcl->~graphics_command_list_d3d11();
		default_deallocate(cl);
		if (is_deferred) // immediate had its refcount adjusted to prevent a circular dependency on the parent device
			release();
	}
}

ref<resource> device::new_resource(const char* name, const resource_desc& desc, memcpy_src* subresource_sources)
{
	static_assert(sizeof(memcpy_src) == sizeof(D3D11_SUBRESOURCE_DATA), "size mismatch");

	auto* d3d = (ID3D11Device*)dev_;
	ID3D11Resource* r = nullptr;

	uint32_t bind  = (desc.resource_bindings & resource_binding::not_shader_resource) || (desc.resource_bindings & resource_binding::constant) ? 0 : D3D11_BIND_SHADER_RESOURCE;

           bind |= (desc.resource_bindings & resource_binding::render_target)       ? D3D11_BIND_RENDER_TARGET    : 0;
           bind |= (desc.resource_bindings & resource_binding::depth_stencil)       ? D3D11_BIND_DEPTH_STENCIL    : 0;
           bind |= (desc.resource_bindings & resource_binding::unordered_access)    ? D3D11_BIND_UNORDERED_ACCESS : 0;
           bind |= (desc.resource_bindings & resource_binding::constant)            ? D3D11_BIND_CONSTANT_BUFFER  : 0;

	uint32_t misc  = 0;
	         misc |= (desc.resource_bindings & resource_binding::unordered_access) 
						    && ((desc.resource_bindings & resource_binding::structured) == 0)   ? (D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS|D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS) : 0;
					 misc |= (desc.resource_bindings & resource_binding::texturecube)         ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0;
					 misc |= (desc.resource_bindings & resource_binding::structured)          ? D3D11_RESOURCE_MISC_BUFFER_STRUCTURED : 0;

	uint32_t array_size = max(uint16_t(1), desc.depth_or_array_size);

	auto usage = from_resource_usage(desc.usage);

	switch (desc.type)
	{
		case resource_type::buffer:
		{
			if ((bind & D3D11_BIND_CONSTANT_BUFFER) == 0 && (misc & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) == 0  && usage != D3D10_USAGE_STAGING) 
				bind |= D3D11_BIND_INDEX_BUFFER|D3D11_BIND_VERTEX_BUFFER;
			CD3D11_BUFFER_DESC d(desc.width, bind, usage, access_flags(usage), misc, (misc & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) ? (desc.width / array_size) : 0);
			oV(d3d->CreateBuffer(&d, (const D3D11_SUBRESOURCE_DATA*)subresource_sources, (ID3D11Buffer**)&r));
			break;
		}

		case resource_type::texture1d:
		{
			oGPU_CHECK(desc.format != surface::format::unknown, "invalid 1d format %s", as_string(desc.format));
			oGPU_CHECK(!surface::is_depth(desc.format), "invalid 1d format %s (cannot be a depth type)", as_string(desc.format));
			CD3D11_TEXTURE1D_DESC d(dxgi::from_surface_format(desc.format), desc.width, array_size, desc.mip_levels, bind, usage, access_flags(usage));
			oV(d3d->CreateTexture1D(&d, (const D3D11_SUBRESOURCE_DATA*)subresource_sources, (ID3D11Texture1D**)&r));
			break;
		}

		case resource_type::texture2d:
		{
			oGPU_CHECK(desc.format != surface::format::unknown, "invalid 2d format %s", as_string(desc.format));
			DXGI_FORMAT tf, df, sf;
			dxgi::get_compatible_formats(dxgi::from_surface_format(desc.format), &tf, &df, &sf);
			CD3D11_TEXTURE2D_DESC d(tf, desc.width, desc.height, array_size, desc.mip_levels, bind, usage, access_flags(usage), desc.sample_count, desc.sample_quality, misc);
			oV(d3d->CreateTexture2D(&d, (const D3D11_SUBRESOURCE_DATA*)subresource_sources, (ID3D11Texture2D**)&r));
			break;
		}

		case resource_type::texture3d:
		{
			oGPU_CHECK(desc.format != surface::format::unknown, "invalid 3d format %s", as_string(desc.format));
			oGPU_CHECK(!surface::is_depth(desc.format), "invalid 3d format %s (cannot be a depth type)", as_string(desc.format));
			CD3D11_TEXTURE3D_DESC d(dxgi::from_surface_format(desc.format), desc.width, desc.height, desc.depth_or_array_size, desc.mip_levels, bind, usage, access_flags(usage));
			oV(d3d->CreateTexture3D(&d, (const D3D11_SUBRESOURCE_DATA*)subresource_sources, (ID3D11Texture3D**)&r));
			break;
		}
	}

	debug_name(r, name);
	return ref<resource>((resource*)r, false);
}

ref<resource> device::new_cb(const char* name, uint32_t struct_stride, uint32_t num_structs)
{
	resource_desc desc = {0};
	desc.width = struct_stride * num_structs;
	desc.depth_or_array_size = (uint16_t)num_structs;
	desc.type = resource_type::buffer;
	desc.usage = resource_usage::upload;
	desc.resource_bindings = resource_binding::constant;
	return new_resource(name, desc);
}

ref<resource> device::new_rb(const char* name, uint32_t struct_stride, uint32_t num_structs)
{
	resource_desc desc = {0};
	desc.width = struct_stride * num_structs;
	desc.depth_or_array_size = (uint16_t)num_structs;
	desc.type = resource_type::buffer;
	desc.usage = resource_usage::readback;
	desc.resource_bindings = resource_binding::not_shader_resource;
	return new_resource(name, desc);
}

ref<resource> device::new_rb(resource* src, bool immediate_copy)
{
	auto copy = d3d::make_cpu_copy((ID3D11Resource*)src, immediate_copy);
	return ref<resource>((resource*)copy.c_ptr());
}

ref<uav> device::new_uav(const char* name, uint32_t struct_stride, uint32_t num_structs, const uav_extension& ext, srv** out_srv)
{
	const bool is_raw = ext == uav_extension::raw;
	oGPU_CHECK(is_raw || sizeof(uint32_t) == struct_stride, "raw buffers must have a struct stride of uint32_t");

	resource_desc desc = {0};
	desc.width = struct_stride * num_structs;
	desc.depth_or_array_size = (uint16_t)num_structs;
	desc.type = resource_type::buffer;
	desc.usage = resource_usage::default;
	desc.resource_bindings = resource_binding::unordered_access | (is_raw ? 0 : resource_binding::structured);
	
	auto r = new_resource(name, desc);

	uav_desc uavd;
	uavd.format = is_raw ? surface::format::r32_typeless : surface::format::unknown;
	uavd.dimension = uav_dimension::buffer;
	uavd.buffer.first_element = 0;
	uavd.buffer.num_elements = num_structs;
	uavd.buffer.structure_byte_stride = struct_stride;
	uavd.buffer.counter_offset_in_bytes = 0;
	uavd.buffer.extension = ext;
	auto view = new_uav(r, uavd);

	if (out_srv)
	{
		srv_desc srvd;
		srvd.format = is_raw ? surface::format::r32_uint : surface::format::unknown;
		srvd.dimension = srv_dimension::buffer;
		srvd.buffer.first_element = 0;
		srvd.buffer.num_elements = num_structs;
		srvd.buffer.structure_byte_stride = struct_stride;
		*out_srv = new_srv(r, srvd);
	}

	return view;
}

ref<srv> device::new_texture(const char* name, const surface::image& src)
{
	auto info = src.info();
	uint32_t mip_levels = info.mips() ? surface::num_mips(info) : 1;

	resource_desc desc;
	desc.width = info.dimensions.x;
	desc.height = info.dimensions.y;
	desc.depth_or_array_size = uint16_t(info.is_array() ? info.array_size : info.dimensions.z);
	desc.mip_levels = (uint16_t)mip_levels;
	desc.alignment = 0;
	desc.sample_count = 1;
	desc.sample_quality = 0;
	desc.type = info.is_1d() ? resource_type::texture1d : (info.is_2d() || info.is_cube() ? resource_type::texture2d : resource_type::texture3d);
	desc.usage = resource_usage::immutable;
	desc.format = info.format;
	desc.layout = texture_layout::standard_swizzle;
	desc.resource_bindings = info.is_cube() ? resource_binding::texturecube : 0;

	auto num_subresources = surface::num_subresources(info);
	auto init_data = (memcpy_src*)alloca(num_subresources * sizeof(memcpy_src));
	for (uint32_t subresource = 0; subresource < num_subresources; subresource++)
	{
		surface::shared_lock lock(src, subresource);
		init_data[subresource].data = lock.mapped.data;
		init_data[subresource].row_pitch = lock.mapped.row_pitch;
		init_data[subresource].slice_pitch = lock.mapped.depth_pitch;
	}

	auto r = new_resource(name, desc, init_data);
	auto view = new_srv(r);
	return view;
}

ibv device::new_ibv(const char* name, uint32_t num_indices, const void* index_data, bool is_32bit)
{
	ibv view;

	const auto bytes = num_indices * surface::element_size(is_32bit ? surface::format::r32_uint : surface::format::r16_uint);
	void* cpu_copy = persistent_mesh_alloc_.allocate(bytes, name ? name : "persistent indices", memory_alignment::align4);
	if (cpu_copy)
	{
		view.offset = (uint32_t)persistent_mesh_alloc_.offset(cpu_copy);
		view.num_indices = num_indices;
		view.transient = 0;
		view.is_32bit = is_32bit;

		copy_box box;
		box.left = view.offset;
		box.top = 0;
		box.front = 0;
		box.right = box.left + bytes;
		box.bottom = 1;
		box.back = 1;

		imm_->update(persistent_mesh_buffer_, 0, &box, index_data, 0, 0);
		memcpy(cpu_copy, index_data, bytes);
	}

	return view;
}

void device::del_ibv(const ibv& view)
{
	// todo: make this deferred and sweep up on present

	if (view.offset)
	{
		void* ptr = persistent_mesh_alloc_.ptr(view.offset);
		persistent_mesh_alloc_.deallocate(ptr);
	}
}

vbv device::new_vbv(const char* name, uint32_t vertex_stride, uint32_t num_vertices, const void* vertex_data)
{
	oGPU_CHECK((vertex_stride & 0x3) == 0, "vertex_stride must be 4-byte aligned");

	vbv view;

	auto bytes = num_vertices * vertex_stride;
	void* cpu_copy = persistent_mesh_alloc_.allocate(bytes, name ? name : "device vertices", memory_alignment::align4);

	if (cpu_copy)
	{
	  view.offset = (uint32_t)persistent_mesh_alloc_.offset(cpu_copy);
		view.vertex_stride_uints_minus_1 = (vertex_stride >> 2) - 1;
		view.transient = 0;
		view.num_vertices = num_vertices;

		copy_box box;
		box.left = view.offset;
		box.top = 0;
		box.front = 0;
		box.right = box.left + bytes;
		box.bottom = 1;
		box.back = 1;

		imm_->update(persistent_mesh_buffer_, 0, &box, vertex_data, 0, 0);
		memcpy(cpu_copy, vertex_data, bytes);
	}

	return view;
}

void device::del_vbv(const vbv& view)
{
	// todo: make this deferred and sweep up on present

	if (view.offset)
	{
		void* ptr = persistent_mesh_alloc_.ptr(view.offset);
		persistent_mesh_alloc_.deallocate(ptr);
	}
}

void device::trace_structs(uint32_t offset, uint32_t struct_stride, uint32_t num_structs, to_string_fn to_string)
{
	uint32_t bytes = struct_stride * num_structs;
	auto cpu_copy = temp_alloc_.scoped_allocate(bytes, "trace_region cpu_copy", memory_alignment::align16);
	ref<resource> temp = new_rb("trace_structs temp", struct_stride, num_structs);

	copy_box box;
	box.left = offset;
	box.top = 0;
	box.front = 0;
	box.right = box.left + bytes;
	box.bottom = 1;
	box.back = 1;

	flush();
	imm_->copy_region(temp, 0, 0, 0, 0, persistent_mesh_buffer_, 0, &box);

	if (!temp->copy_to(cpu_copy))
		oTrace("vbv readback failed");

	char buf[1024];
	auto cur = (const char*)cpu_copy;
	for (uint32_t i = 0; i < num_structs; i++)
	{
		if (!to_string(buf, countof(buf), cur))
			strlcpy(buf, "<parse-fail>");
		oTrace("%u %s", i, buf);
		cur += struct_stride;
	}
}

ref<timer> device::new_timer(const char* name)
{
	auto d3d = (ID3D11Device*)dev_;
	auto t = (timer_d3d11*)default_allocate(sizeof(timer_d3d11), "timer_d3d11");

	D3D11_QUERY_DESC start_stop_desc = { D3D11_QUERY_TIMESTAMP,          0 };
	D3D11_QUERY_DESC disjoint_desc   = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };

	oV(d3d->CreateQuery(&start_stop_desc, &t->start));
	oV(d3d->CreateQuery(&start_stop_desc, &t->stop));
	oV(d3d->CreateQuery(&disjoint_desc,   &t->disjoint));
	t->dev = this;
	t->refcount = 1;
	reference();
	return ref<timer>((timer*)t, false);
}

void device::del_timer(timer* t)
{
	if (t)
	{
		((timer_d3d11*)t)->~timer_d3d11();
		default_deallocate(t);
		release();
	}
}

ref<occlusion_query> device::new_occlusion_query(const char* name)
{
	auto d3d = (ID3D11Device*)dev_;
	D3D11_QUERY_DESC d = { D3D11_QUERY_OCCLUSION, 0 };
	ID3D11Query* q = nullptr;
	oV(d3d->CreateQuery(&d, &q));
	debug_name(q, name);
	return ref<occlusion_query>((occlusion_query*)q, false);
}

ref<fence> device::new_fence(const char* name)
{
	auto d3d = (ID3D11Device*)dev_;
	D3D11_QUERY_DESC d = { D3D11_QUERY_EVENT, 0 };
	ID3D11Query* q = nullptr;
	oV(d3d->CreateQuery(&d, &q));
	debug_name(q, name);
	return ref<fence>((fence*)q, false);
}

ref<stats_query> device::new_stats_query(const char* name)
{
	auto d3d = (ID3D11Device*)dev_;
	D3D11_QUERY_DESC d = { D3D11_QUERY_PIPELINE_STATISTICS, 0 };
	ID3D11Query* q = nullptr;
	oV(d3d->CreateQuery(&d, &q));
	debug_name(q, name);
	return ref<stats_query>((stats_query*)q, false);
}

ref<rtv> device::new_rtv(resource* r, const rtv_desc& desc)
{
	ID3D11RenderTargetView* v = nullptr;
	if (r)
	{
		auto d3d = (ID3D11Device*)dev_;
		auto d = from_rtv_desc(desc);
		oV(d3d->CreateRenderTargetView((ID3D11Resource*)r, &d, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<rtv>((rtv*)v, false);
}

ref<dsv> device::new_dsv(resource* r, const dsv_desc& desc)
{
	ID3D11DepthStencilView* v = nullptr;
	if (r)
	{
		auto d3d = (ID3D11Device*)dev_;
		auto d = from_dsv_desc(desc);
		oV(d3d->CreateDepthStencilView((ID3D11Resource*)r, &d, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<dsv>((dsv*)v, false);
}

ref<uav> device::new_uav(resource* r, const uav_desc& desc)
{
	ID3D11UnorderedAccessView* v = nullptr;
	if (r)
	{
		auto d3d = (ID3D11Device*)dev_;
		auto d = from_uav_desc(desc);
		oV(d3d->CreateUnorderedAccessView((ID3D11Resource*)r, &d, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<uav>((uav*)v, false);
}

ref<srv> device::new_srv(resource* r, const srv_desc& desc)
{
	ID3D11ShaderResourceView* v = nullptr;
	if (r)
	{
		auto d3d = (ID3D11Device*)dev_;
		auto d = from_srv_desc(desc);
		oV(d3d->CreateShaderResourceView((ID3D11Resource*)r, &d, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<srv>((srv*)v, false);
}

ref<rtv> device::new_rtv(resource* r)
{
	ID3D11RenderTargetView* v = nullptr;
	if (r)
	{
		auto d3d = (ID3D11Device*)dev_;
		oV(d3d->CreateRenderTargetView((ID3D11Resource*)r, nullptr, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<rtv>((rtv*)v, false);
}

ref<dsv> device::new_dsv(resource* r)
{
	ID3D11DepthStencilView* v = nullptr;
	if (r)
	{
		auto d3d = (ID3D11Device*)dev_;
		auto desc = r->get_desc();
		dsv_desc dsvd;
		dsvd.format = surface::as_depth(desc.format);
		dsvd.dimension = dsv_dimension::texture2d;
		dsvd.texture2d.mip_slice = 0;
		auto dsvd_d3d11 = from_dsv_desc(dsvd);

		oV(d3d->CreateDepthStencilView((ID3D11Resource*)r, &dsvd_d3d11, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<dsv>((dsv*)v, false);
}

ref<uav> device::new_uav(resource* r)
{
	ID3D11UnorderedAccessView* v = nullptr;
	if (r)
	{
		auto d3d = (ID3D11Device*)dev_;
		oV(d3d->CreateUnorderedAccessView((ID3D11Resource*)r, nullptr, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<uav>((uav*)v, false);
}

ref<srv> device::new_srv(resource* r)
{
	ID3D11ShaderResourceView* v = nullptr;
	if (r)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC* p_desc = nullptr;
		D3D11_SHADER_RESOURCE_VIEW_DESC desc;

		auto d = r->get_desc();
		if (surface::is_typeless(d.format))
		{
			desc = srv_from_resource_desc(d);
			p_desc = &desc;
		}

		auto d3d = (ID3D11Device*)dev_;
		oV(d3d->CreateShaderResourceView((ID3D11Resource*)r, p_desc, &v));
		char tmp[128];
		debug_name(v, r->name(tmp));
	}
	return ref<srv>((srv*)v, false);
}

ref<rtv> device::new_rtv(const char* name, uint32_t width, uint32_t height, const surface::format& format)
{
	resource_desc desc;
	desc.width = width;
	desc.height = height;
	desc.depth_or_array_size = 0;
	desc.mip_levels = 1;
	desc.alignment = 0;
	desc.sample_count = 1;
	desc.sample_quality = 0;
	desc.type = resource_type::texture2d;
	desc.usage = resource_usage::default;
	desc.format = format;
	desc.layout = texture_layout::standard_swizzle;
	desc.resource_bindings = resource_binding::render_target;
	return new_rtv(new_resource(name, desc));
}

ref<dsv> device::new_dsv(const char* name, uint32_t width, uint32_t height, const surface::format& format)
{
	resource_desc desc;
	desc.width = width;
	desc.height = height;
	desc.depth_or_array_size = 0;
	desc.mip_levels = 1;
	desc.alignment = 0;
	desc.sample_count = 1;
	desc.sample_quality = 0;
	desc.type = resource_type::texture2d;
	desc.usage = resource_usage::default;
	desc.format = format;
	desc.layout = texture_layout::standard_swizzle;
	desc.resource_bindings = resource_binding::depth_stencil;
	return new_dsv(new_resource(name, desc));
}

void device::resolve_transient_fences()
{
	// Go through all prior fences and forward the end pointer
	// as fences permit.

	const uint32_t cur_i = transient_fence_index_;
	uint32_t i = cur_i;
	do
	{
		// fences are issued in order, so the first one that's not done 
		// implies the rest aren't done.
		if (!transient_fences_[i]->finished(false))
			break;

		// update the current state of the ring buffer
		transient_ring_end_ = next_transient_ring_ends_[i];

		// mark the next end clear for use (and fence clear for reissue)
		next_transient_ring_ends_[i] = 0;

		// move to the next entry
		i = (i + 1) & 0x3;
	
	} while (i != cur_i);
}

void device::insert_transient_fence()
{
	// more than 4 frames latent if this is true
	if (!next_transient_ring_ends_[transient_fence_index_])
	{
		// kick a fence to notify when we've consumed all allocations since resolve_transient_fences()
		next_transient_ring_ends_[transient_fence_index_] = transient_ring_start_;
		imm_->insert_fence(transient_fences_[transient_fence_index_]);
		transient_fence_index_ = (transient_fence_index_ + 1) & 0x3;
	}
}

bool device::allocate_transient_mesh(graphics_command_list* cl, uint32_t bytes, void** out_ptr, uint32_t* out_offset)
{
	*out_ptr = nullptr;
	*out_offset = ~0u;

	const uint32_t aligned_bytes = align(bytes, sizeof(uint32_t));
	const uint32_t end = transient_ring_end_;
	const uint32_t buffer_size = init_.max_transient_mesh_bytes;

	uint32_t old_start, new_start, offset;
	do
	{
		old_start = transient_ring_start_.load();
		offset = old_start;
		new_start = old_start + aligned_bytes;

		// if the ring buffer end is behind us, check the end of the buffer size and wrap
		if (new_start >= buffer_size)
		{
			offset = 0;
			new_start = aligned_bytes;
		}

		if (old_start < end && new_start > end)
			return nullptr;

	} while (!transient_ring_start_.compare_exchange_strong(old_start, new_start));

	memcpy_dest mapped;
	if (!cl->map(&mapped, transient_mesh_buffer_, 0, map_type::no_overwrite))
		return false;

	*out_ptr = (uint8_t*)mapped.data + offset;
	*out_offset = offset;
	return true;
}

void graphics_command_list::reference()
{
	auto gcl = (graphics_command_list_d3d11*)this;
	++gcl->refcount;
}

void graphics_command_list::release()
{
	auto gcl = (graphics_command_list_d3d11*)this;
	if (--gcl->refcount == 0)
	{
		auto dev = ((graphics_command_list_d3d11*)this)->dev;
		dev->del_graphics_command_list(this);
	}
}

void graphics_command_list::set_marker(const char* marker)
{
	auto uda = ((graphics_command_list_d3d11*)this)->uda;
	uda->SetMarker(lwstring(marker));
}

void graphics_command_list::push_marker(const char* marker)
{
	auto uda = ((graphics_command_list_d3d11*)this)->uda;
	uda->BeginEvent(lwstring(marker));
}

void graphics_command_list::pop_marker()
{
	auto uda = ((graphics_command_list_d3d11*)this)->uda;
	uda->EndEvent();
}

device* graphics_command_list::device()
{
	return ((graphics_command_list_d3d11*)this)->dev;
}

void graphics_command_list::begin_timer(timer* t)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	auto tm = (timer_d3d11*)t;
	dc->Begin(tm->disjoint);
	dc->End(tm->start);
}

void graphics_command_list::end_timer(timer* t)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	auto tm = (timer_d3d11*)t;
	dc->End(tm->stop);
	dc->End(tm->disjoint);
}

void graphics_command_list::begin_occlusion_query(occlusion_query* q)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->Begin((ID3D11Asynchronous*)q);
}

void graphics_command_list::end_occlusion_query(occlusion_query* q)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->End((ID3D11Asynchronous*)q);
}

void graphics_command_list::insert_fence(fence* f)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->End((ID3D11Asynchronous*)f);
}

void graphics_command_list::clear_dsv(dsv* view, const clear_type& clear_type, float depth, uint8_t stencil)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->ClearDepthStencilView((ID3D11DepthStencilView*)view, from_clear_type(clear_type), depth, stencil);
}

void graphics_command_list::clear_rtv(rtv* view, const float color_rgba[4])
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->ClearRenderTargetView((ID3D11RenderTargetView*)view, color_rgba);
}

void graphics_command_list::clear_uav_float(uav* view, const float values[4])
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->ClearUnorderedAccessViewFloat((ID3D11UnorderedAccessView*)view, values);
}

void graphics_command_list::clear_uav_uint(uav* view, const uint32_t values[4])
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->ClearUnorderedAccessViewUint((ID3D11UnorderedAccessView*)view, values);
}

void* graphics_command_list::new_transient_indices(uint32_t num_indices, bool is_32bit)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	const auto bytes = align(num_indices * surface::element_size(is_32bit ? surface::format::r32_uint : surface::format::r16_uint), sizeof(uint32_t));
	void* ptr;
	if (gcl->dev->allocate_transient_mesh(this, bytes, &ptr, &gcl->transient_offset))
	{
		gcl->num_transient_elements = num_indices;
		gcl->transient_stride_or_32bit = is_32bit ? 1 : 0;
	}
	return ptr;
}

ibv graphics_command_list::commit_transient_indices()
{
	auto gcl = (graphics_command_list_d3d11*)this;
	unmap(gcl->mesh_buffers[1]);

	ibv view;
	view.offset = gcl->transient_offset;
	view.num_indices = gcl->num_transient_elements;
	view.transient = 1;
	view.is_32bit = gcl->transient_stride_or_32bit != 0;
	return view;
}

void* graphics_command_list::new_transient_vertices(uint32_t vertex_stride, uint32_t num_vertices)
{
	if ((vertex_stride & 0x3) != 0)
		oThrow(std::errc::invalid_argument, "vertex_stride must be 4-byte aligned");

	auto gcl = (graphics_command_list_d3d11*)this;
	const auto bytes = vertex_stride * num_vertices;
	
	void* ptr;
	if (gcl->dev->allocate_transient_mesh(this, bytes, &ptr, &gcl->transient_offset))
	{
		gcl->num_transient_elements = num_vertices;
		gcl->transient_stride_or_32bit = vertex_stride;
	}
	return ptr;
}

vbv graphics_command_list::commit_transient_vertices()
{
	auto gcl = (graphics_command_list_d3d11*)this;
	unmap(gcl->mesh_buffers[1]);

	vbv view;
	view.offset = gcl->transient_offset;
	view.num_vertices = gcl->num_transient_elements;
	view.transient = 1;
	view.vertex_stride_bytes(gcl->transient_stride_or_32bit);
	return view;
}

void graphics_command_list::dispatch(uint32_t thread_group_x, uint32_t thread_group_y, uint32_t thread_group_z)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->Dispatch(thread_group_x, thread_group_y, thread_group_z);
}

void graphics_command_list::dispatch_indirect(resource* args_buffer, uint32_t args_offset)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->DispatchIndirect((ID3D11Buffer*)args_buffer, args_offset);
}

void graphics_command_list::draw_indexed(uint32_t num_indices_per_instance, uint32_t num_instances, uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;

	if (num_instances == 1)
		dc->DrawIndexed(num_indices_per_instance, start_index_location, base_vertex_location);
	else if (num_instances > 1)
		dc->DrawIndexedInstanced(num_indices_per_instance, num_instances, start_index_location, base_vertex_location, start_instance_location);
}

void graphics_command_list::draw(uint32_t num_vertices, uint32_t num_instances, uint32_t start_vertex_location, uint32_t start_instance_location)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	if (num_instances == 1)
		dc->Draw(num_vertices, start_vertex_location);
	else if (num_instances > 1)
		dc->DrawInstanced(num_vertices, num_instances, start_vertex_location, start_instance_location);
}

void graphics_command_list::set_rtvs(uint32_t num_rtvs, rtv* const* rtvs, dsv* dsv, uint32_t num_viewports, const viewport_desc* viewports, uint32_t uav_start, uint32_t num_uavs, uav* const* uavs, const uint32_t* initial_counts)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->OMSetRenderTargetsAndUnorderedAccessViews(num_rtvs, (ID3D11RenderTargetView* const*)rtvs, (ID3D11DepthStencilView*)dsv, uav_start, num_uavs, (ID3D11UnorderedAccessView* const*)uavs, initial_counts);
	if (num_viewports)
	{
		dc->RSSetViewports(num_viewports, (const D3D11_VIEWPORT*)viewports);

		D3D11_RECT rects[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		for (uint32_t i = 0; i < num_viewports; i++)
		{
			rects[i].left = (int)viewports[i].top_left_x;
			rects[i].top = (int)viewports[i].top_left_y;
			rects[i].right = rects[i].left + (int)viewports[i].width;
			rects[i].bottom = rects[i].top + (int)viewports[i].height;
		}

		dc->RSSetScissorRects(num_viewports, rects);
	}

	else if (rtvs && rtvs[0])
	{
		auto desc = rtvs[0]->get_resource()->get_desc();

		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (float)desc.width;
		vp.Height = (float)desc.height;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		dc->RSSetViewports(num_rtvs, &vp);

		D3D11_RECT rect = { 0, 0, (LONG)desc.width, (LONG)desc.height };
		dc->RSSetScissorRects(1, &rect);
	}

	else
	{
		// any pixel shader operation will quitely noop if a viewport isn't specified
		// so set a 1x1 if none other is set.
		uint32_t nViewports = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
		D3D11_VIEWPORT Viewports[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE];
		dc->RSGetViewports(&nViewports, Viewports);
		if (!nViewports)
		{
			D3D11_VIEWPORT& v = Viewports[0];
			v.TopLeftX = 0.0f;
			v.TopLeftY = 0.0f;
			v.Width = 1.0f;
			v.Height = 1.0f;
			v.MinDepth = 0.0f;
			v.MaxDepth = 1.0f;
			dc->RSSetViewports(1, Viewports);

			D3D11_RECT rect = { 0, 0, 1, 1 };
			dc->RSSetScissorRects(1, &rect);
		}
	}
}

void graphics_command_list::set_uavs(uint32_t uav_start, uint32_t num_uavs, uav* const* uavs, const uint32_t* initial_counts)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->CSSetUnorderedAccessViews(uav_start, num_uavs, (ID3D11UnorderedAccessView* const*)uavs, initial_counts);
}

void graphics_command_list::reset(rso* root_signature, pso* pipeline_state)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	auto dc = gcl->dc;
	dc->ClearState();
	gcl->current_rso = nullptr;
	gcl->current_pso = nullptr;
	gcl->current_cso = nullptr;
	memset(gcl->blend_factor, 0, sizeof(gcl->blend_factor));
	gcl->stencil_ref = 0;

	if (root_signature)
		set_rso(root_signature);

	if (pipeline_state)
		set_pso(pipeline_state);
}

void graphics_command_list::set_rso(rso* o, uint32_t stage_bindings)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	auto oo = (rso_d3d11*)o;
	if (oo != gcl->current_rso)
	{
		auto dc = gcl->dc;
		d3d::set_samplers(dc, 0, (uint32_t)oo->samplers.size(), (ID3D11SamplerState* const*)oo->samplers.data(), stage_bindings);

		d3d::set_cbuffers(dc, 0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT, (ID3D11Buffer* const*)s_null_memory, stage_bindings);
		gcl->bound_cbuffers.fill(nullptr);
		gcl->current_rso = oo;
	}
}

void graphics_command_list::set_rso(uint32_t rso_index, uint32_t stage_bindings)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	oGPU_CHECK(rso_index < gcl->max_rsos, "invalid rso_index %u (out of range, last allowable index %u)", rso_index, gcl->max_rsos - 1);
	rso* o = gcl->rsos[rso_index];
	oGPU_CHECK(o, "invalid rso_index (%u) there is no rso registered", rso_index);
	set_rso(o, stage_bindings);
}

void graphics_command_list::set_pso(pso* o)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	pso_d3d11* oo = o ? (pso_d3d11*)o : &sNullPSO;
	if (oo != gcl->current_pso)
	{
		auto dc = gcl->dc;
		dc->VSSetShader(oo->vs, nullptr, 0);
		dc->HSSetShader(oo->hs, nullptr, 0);
		dc->DSSetShader(oo->ds, nullptr, 0);
		dc->GSSetShader(oo->gs, nullptr, 0);
		dc->PSSetShader(oo->ps, nullptr, 0);
		dc->IASetInputLayout(oo->ia);
		dc->IASetPrimitiveTopology(oo->primitive_topology);
		dc->RSSetState(oo->rasterizer_state);
		dc->OMSetBlendState(oo->blend_state, gcl->blend_factor, oo->sample_mask);
		dc->OMSetDepthStencilState(oo->depth_stencil_state, gcl->stencil_ref);
		gcl->current_pso = oo;
	}
}

void graphics_command_list::set_pso(uint32_t pso_index)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	oGPU_CHECK(pso_index < gcl->max_psos, "invalid pso_index %u (out of range, last allowable index %u)", pso_index, gcl->max_psos - 1);
	pso* o = gcl->psos[pso_index];
	oGPU_CHECK(o, "invalid pso_index (%u) there is no pso registered", pso_index);
	set_pso(o);
}

void graphics_command_list::set_cso(cso* o)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	cso_d3d11* oo = o ? (cso_d3d11*)o : &sNullCSO;
	if (oo != gcl->current_cso)
	{
		auto dc = gcl->dc;
		dc->CSSetShader(oo->cs, nullptr, 0);
		gcl->current_cso = oo;
	}
}

void graphics_command_list::set_cso(uint32_t cso_index)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	oGPU_CHECK(cso_index < gcl->max_psos, "invalid cso_index %u (out of range, last allowable index %u)", cso_index, gcl->max_csos - 1);
	cso* o = gcl->csos[cso_index];
	oGPU_CHECK(o, "invalid cso_index (%u) there is no pso registered", cso_index);
	set_cso(o);
}

void graphics_command_list::set_cbv(uint32_t cbv_index, const void* src, uint32_t src_size, uint32_t stage_bindings)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	auto dc = gcl->dc;
	auto rso = gcl->current_rso;
	oGPU_CHECK(rso, "An appropriate root signature must be set before modifying cbvs");
	auto fitted = rso->cbuffers[cbv_index];
	auto cbuffer = fitted.best_fit(src_size / fitted.struct_stride());

	oGPU_CHECK(cbuffer, "no fit for cbuffer in slot %u of src_size %u", cbv_index, src_size);
	D3D11_MAPPED_SUBRESOURCE mapped;
	oV(dc->Map(cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
	oGPU_CHECK(mapped.pData, "mapped buffer is null");
	memcpy(mapped.pData, src, src_size);
	dc->Unmap(cbuffer, 0);

	if (gcl->bound_cbuffers[cbv_index] != cbuffer)
	{
		gcl->bound_cbuffers[cbv_index] = cbuffer;
		d3d::set_cbuffers(dc, cbv_index, 1, &cbuffer, stage_bindings);
	}
}

void graphics_command_list::set_srvs(uint32_t srv_start, uint32_t num_srvs, const srv* const* srvs, uint32_t stage_bindings)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	srvs = srvs ? srvs : (const srv* const*)s_null_memory;
	d3d::set_srvs(dc, srv_start, num_srvs, (ID3D11ShaderResourceView* const*)srvs, stage_bindings);
}

void graphics_command_list::set_blend_factor(float blend_factor[4])
{
	auto gcl = (graphics_command_list_d3d11*)this;

	if (memcmp(gcl->blend_factor, blend_factor, sizeof(gcl->blend_factor)))
	{
		auto dc = gcl->dc;
		auto o = gcl->current_pso ? gcl->current_pso : &sNullPSO;
		dc->OMSetBlendState(o->blend_state, gcl->blend_factor, o->sample_mask);
		memcpy(gcl->blend_factor, blend_factor, sizeof(gcl->blend_factor));
	}
}

void graphics_command_list::set_stencil_ref(uint32_t stencil_ref)
{
	auto gcl = (graphics_command_list_d3d11*)this;

	if (gcl->stencil_ref != stencil_ref)
	{
		auto dc = gcl->dc;
		auto o = gcl->current_pso ? gcl->current_pso : &sNullPSO;
		dc->OMSetDepthStencilState(o->depth_stencil_state, gcl->stencil_ref);
		gcl->stencil_ref = stencil_ref;
	}
}

void graphics_command_list::set_indices(const ibv& view)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	auto buf = ((graphics_command_list_d3d11*)this)->mesh_buffers[view.transient];
	dc->IASetIndexBuffer((ID3D11Buffer*)buf, view.is_32bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT, view.offset);
}

void graphics_command_list::set_vertices(uint32_t vbv_start, uint32_t num_vbvs, const vbv* vbvs)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	if (vbvs)
	{
		ID3D11Buffer* vbuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		uint32_t strides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		uint32_t offsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
		for (uint32_t i = 0; i < num_vbvs; i++)
		{
			auto buf = ((graphics_command_list_d3d11*)this)->mesh_buffers[vbvs[i].transient];
			vbuffers[i] = (ID3D11Buffer*)buf;
			strides[i] = vbvs[i].vertex_stride_bytes();
			offsets[i] = vbvs[i].offset;
		}
		dc->IASetVertexBuffers(0, num_vbvs, vbuffers, strides, offsets);
	}

	else
		dc->IASetVertexBuffers(0, num_vbvs, (ID3D11Buffer* const*)s_null_memory, (const uint32_t*)s_null_memory, (const uint32_t*)s_null_memory);
}

bool graphics_command_list::map(memcpy_dest* mapped, resource* r, uint32_t subresource, map_type type)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	D3D11_MAPPED_SUBRESOURCE msr;
	if (SUCCEEDED(dc->Map((ID3D11Resource*)r, subresource, /*type == map_type::discard ? */D3D11_MAP_WRITE_DISCARD/* : D3D11_MAP_WRITE_NO_OVERWRITE*/, 0, &msr)) && msr.pData)
	{
		mapped->data = msr.pData;
		mapped->row_pitch = msr.RowPitch;
		mapped->slice_pitch = msr.DepthPitch;
		return true;
	}
	return false;
}

void graphics_command_list::unmap(resource* r, uint32_t subresource)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->Unmap((ID3D11Resource*)r, subresource);
}

void graphics_command_list::update(resource* r, uint32_t subresource, const copy_box* box, const void* src, uint32_t src_row_pitch, uint32_t src_depth_pitch)
{
	auto gcl = (graphics_command_list_d3d11*)this;
	auto dc = gcl->dc;
	auto device_supports_deferred_contexts = gcl->supports_deferred_contexts;

	D3D11_BOX local_box;
	D3D11_BOX* pbox = nullptr;
	if (box)
	{
		local_box.left   = box->left;
		local_box.top    = box->top;
		local_box.right  = box->right;
		local_box.bottom = box->bottom; 
		local_box.front  = box->front;
		local_box.back   = box->back;
		pbox = &local_box;
	}

	uint8_t* adjusted_data = (uint8_t*)src;

	if (pbox && !device_supports_deferred_contexts)
	{
		auto desc = r->get_desc();

		if (surface::is_block_compressed(desc.format))
		{
			pbox->left   /= 4;
			pbox->right  /= 4;
			pbox->top    /= 4;
			pbox->bottom /= 4;
		}

		adjusted_data -= (pbox->front * src_depth_pitch) - (pbox->top * src_row_pitch) - (pbox->left * surface::element_size(desc.format));
	}

	dc->UpdateSubresource((ID3D11Resource*)r, subresource, pbox, src, src_row_pitch, src_depth_pitch);
}

void graphics_command_list::copy(resource* dest, resource* src)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->CopyResource((ID3D11Resource*)dest, (ID3D11Resource*)src);
}

void graphics_command_list::copy_region(resource* dest, uint32_t dest_subresource, uint32_t dest_x, uint32_t dest_y, uint32_t dest_z, resource* src, uint32_t src_subresource, const copy_box* src_box)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->CopySubresourceRegion((ID3D11Resource*)dest, dest_subresource, dest_x, dest_y, dest_z, (ID3D11Resource*)src, src_subresource, (const D3D11_BOX*)src_box);
}

void graphics_command_list::copy_structure_count(resource* dest, uint32_t dest_offset_uints, uav* src)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->CopyStructureCount((ID3D11Buffer*)dest, dest_offset_uints * sizeof(uint32_t), (ID3D11UnorderedAccessView*)src);
}

void graphics_command_list::generate_mips(srv* view)
{
	auto dc = ((graphics_command_list_d3d11*)this)->dc;
	dc->GenerateMips((ID3D11ShaderResourceView*)view);
}

}}
