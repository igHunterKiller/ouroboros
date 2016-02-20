// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "d3d11_as_string.h"

namespace ouro {

template<> const char* as_string(const D3D11_BIND_FLAG& flag)
{
	switch (flag)
	{
		case D3D11_BIND_VERTEX_BUFFER: return "D3D11_BIND_VERTEX_BUFFER";
		case D3D11_BIND_INDEX_BUFFER: return "D3D11_BIND_INDEX_BUFFER";
		case D3D11_BIND_CONSTANT_BUFFER: return "D3D11_BIND_CONSTANT_BUFFER";
		case D3D11_BIND_SHADER_RESOURCE: return "D3D11_BIND_SHADER_RESOURCE";
		case D3D11_BIND_STREAM_OUTPUT: return "D3D11_BIND_STREAM_OUTPUT";
		case D3D11_BIND_RENDER_TARGET: return "D3D11_BIND_RENDER_TARGET";
		case D3D11_BIND_DEPTH_STENCIL: return "D3D11_BIND_DEPTH_STENCIL";
		case D3D11_BIND_UNORDERED_ACCESS: return "D3D11_BIND_UNORDERED_ACCESS";
		//case D3D11_BIND_DECODER: return "D3D11_BIND_DECODER";
		//case D3D11_BIND_VIDEO_ENCODER: return "D3D11_BIND_VIDEO_ENCODER";
		default: break;
	}
	return "?";
}

template<> const char* as_string(const D3D11_CPU_ACCESS_FLAG& flag)
{
	switch (flag)
	{
		case D3D11_CPU_ACCESS_WRITE: return "D3D11_CPU_ACCESS_WRITE";
		case D3D11_CPU_ACCESS_READ: return "D3D11_CPU_ACCESS_READ";
		default: break;
	}
	return "?";
}

template<> const char* as_string(const D3D11_RESOURCE_MISC_FLAG& flag)
{
	switch (flag)
	{
		case D3D11_RESOURCE_MISC_GENERATE_MIPS: return "D3D11_RESOURCE_MISC_GENERATE_MIPS";
		case D3D11_RESOURCE_MISC_SHARED: return "D3D11_RESOURCE_MISC_SHARED";
		case D3D11_RESOURCE_MISC_TEXTURECUBE: return "D3D11_RESOURCE_MISC_TEXTURECUBE";
		case D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS: return "D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS";
		case D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS: return "D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS";
		case D3D11_RESOURCE_MISC_BUFFER_STRUCTURED: return "D3D11_RESOURCE_MISC_BUFFER_STRUCTURED";
		case D3D11_RESOURCE_MISC_RESOURCE_CLAMP: return "D3D11_RESOURCE_MISC_RESOURCE_CLAMP";
		case D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX: return "D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX";
		case D3D11_RESOURCE_MISC_GDI_COMPATIBLE: return "D3D11_RESOURCE_MISC_GDI_COMPATIBLE";
		//case D3D11_RESOURCE_MISC_SHARED_NTHANDLE: return "D3D11_RESOURCE_MISC_SHARED_NTHANDLE";
		//case D3D11_RESOURCE_MISC_RESTRICTED_CONTENT: return "D3D11_RESOURCE_MISC_RESTRICTED_CONTENT";
		//case D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE: return "D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE";
		//case D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER: return "D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER";
		default: break;
	}
	return "?";
}

template<> const char* as_string(const D3D11_RESOURCE_DIMENSION& type)
{
	switch (type)
	{
		case D3D11_RESOURCE_DIMENSION_UNKNOWN: return "D3D11_RESOURCE_DIMENSION_UNKNOWN	=";
		case D3D11_RESOURCE_DIMENSION_BUFFER: return "D3D11_RESOURCE_DIMENSION_BUFFER	=";
		case D3D11_RESOURCE_DIMENSION_TEXTURE1D: return "D3D11_RESOURCE_DIMENSION_TEXTURE1D	=";
		case D3D11_RESOURCE_DIMENSION_TEXTURE2D: return "D3D11_RESOURCE_DIMENSION_TEXTURE2D	=";
		case D3D11_RESOURCE_DIMENSION_TEXTURE3D: return "D3D11_RESOURCE_DIMENSION_TEXTURE3D	=";
		default: break;
	}
	return "?";
}

template<> const char* as_string(const D3D11_UAV_DIMENSION& type)
{
	switch (type)
	{
		case D3D11_UAV_DIMENSION_UNKNOWN: return "D3D11_UAV_DIMENSION_UNKNOWN";
		case D3D11_UAV_DIMENSION_BUFFER: return "D3D11_UAV_DIMENSION_BUFFER";
		case D3D11_UAV_DIMENSION_TEXTURE1D: return "D3D11_UAV_DIMENSION_TEXTURE1D";
		case D3D11_UAV_DIMENSION_TEXTURE1DARRAY: return "D3D11_UAV_DIMENSION_TEXTURE1DARRAY";
		case D3D11_UAV_DIMENSION_TEXTURE2D: return "D3D11_UAV_DIMENSION_TEXTURE2D";
		case D3D11_UAV_DIMENSION_TEXTURE2DARRAY: return "D3D11_UAV_DIMENSION_TEXTURE2DARRAY";
		case D3D11_UAV_DIMENSION_TEXTURE3D: return "D3D11_UAV_DIMENSION_TEXTURE3D";
		default: break;
	}
	return "?";
}

template<> const char* as_string(const D3D11_USAGE& usage)
{
	switch (usage)
	{
		case D3D11_USAGE_DEFAULT: return "D3D11_USAGE_DEFAULT";
		case D3D11_USAGE_IMMUTABLE: return "D3D11_USAGE_IMMUTABLE";
		case D3D11_USAGE_DYNAMIC: return "D3D11_USAGE_DYNAMIC";
		case D3D11_USAGE_STAGING: return "D3D11_USAGE_STAGING";
		default: break;
	}
	return "?";
}

}
