// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Shaders to generate the input signature required for defining a vertex layout
// without other dependencies.

#include <oGfx/vertex_layouts.h>

#define SIG(type) float4 VSSig_##type(uint instance : SV_InstanceID, VTX##type In) oSEM_VS_POSITION { return float4(0, 0, 0, 1); }

SIG(pos)
SIG(pos_nrm_tan)
SIG(pos_uv0)
SIG(pos_uvw)
SIG(pos_col)
SIG(pos_uv0_col)
SIG(pos_nrm_tan_uv0)
SIG(pos_nrm_tan_uvw)
SIG(pos_nrm_tan_col)
SIG(pos_nrm_tan_uv0_uv1)
SIG(pos_nrm_tan_uv0_col)
SIG(pos_nrm_tan_uvw_col)
SIG(pos_nrm_tan_uv0_uv1_col)
SIG(uv0)
SIG(uv0_uv1)
SIG(uvw)
SIG(col)
SIG(uv0_col)
SIG(uv0_uv1_col)
SIG(uvw_col)
