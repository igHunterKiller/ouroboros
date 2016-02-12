// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oGfx/vertex_layouts.h>
#include <oSurface/surface.h>
#include "bytecode_util.h"

#if 0

#include <VSSig_pos.h>
#include <VSSig_pos_nrm_tan.h>
#include <VSSig_pos_uv0.h>
#include <VSSig_pos_uvw.h>
#include <VSSig_pos_col.h>
#include <VSSig_pos_uv0_col.h>
#include <VSSig_pos_nrm_tan_uv0.h>
#include <VSSig_pos_nrm_tan_uvw.h>
#include <VSSig_pos_nrm_tan_col.h>
#include <VSSig_pos_nrm_tan_uv0_uv1.h>
#include <VSSig_pos_nrm_tan_uv0_col.h>
#include <VSSig_pos_nrm_tan_uvw_col.h>
#include <VSSig_pos_nrm_tan_uv0_uv1_col.h>
#include <VSSig_uv0.h>
#include <VSSig_uv0_uv1.h>
#include <VSSig_uvw.h>
#include <VSSig_col.h>
#include <VSSig_uv0_col.h>
#include <VSSig_uv0_uv1_col.h>
#include <VSSig_uvw_col.h>

namespace ouro { namespace gfx {

oSH_BEGIN(vertex_layout)
{
	oSH_NULL,
	oSH(VSSig_pos),
	oSH(VSSig_pos_nrm_tan),
	oSH(VSSig_pos_uv0),
	oSH(VSSig_pos_uvw),
	oSH(VSSig_pos_col),
	oSH(VSSig_pos_uv0_col),
	oSH(VSSig_pos_nrm_tan_uv0),
	oSH(VSSig_pos_nrm_tan_uvw),
	oSH(VSSig_pos_nrm_tan_col),
	oSH(VSSig_pos_nrm_tan_uv0_uv1),
	oSH(VSSig_pos_nrm_tan_uv0_col),
	oSH(VSSig_pos_nrm_tan_uvw_col),
	oSH(VSSig_pos_nrm_tan_uv0_uv1_col),
	oSH(VSSig_uv0),
	oSH(VSSig_uv0_uv1),
	oSH(VSSig_uvw),
	oSH(VSSig_col),
	oSH(VSSig_uv0_col),
	oSH(VSSig_uv0_uv1_col),
	oSH(VSSig_uvw_col),
};
oSH_END_SIG(vertex_layout)

}}
#endif