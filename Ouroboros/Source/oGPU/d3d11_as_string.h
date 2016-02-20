// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <d3d11.h>

namespace ouro {

template<> const char* as_string<D3D11_BIND_FLAG>(const D3D11_BIND_FLAG& flag);
template<> const char* as_string<D3D11_CPU_ACCESS_FLAG>(const D3D11_CPU_ACCESS_FLAG& flag);
template<> const char* as_string<D3D11_RESOURCE_MISC_FLAG>(const D3D11_RESOURCE_MISC_FLAG& flag);
template<> const char* as_string<D3D11_RESOURCE_DIMENSION>(const D3D11_RESOURCE_DIMENSION& type);
template<> const char* as_string<D3D11_UAV_DIMENSION>(const D3D11_UAV_DIMENSION& type);
template<> const char* as_string<D3D11_USAGE>(const D3D11_USAGE& usage);

}
