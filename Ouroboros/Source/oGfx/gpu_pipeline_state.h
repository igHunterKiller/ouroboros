// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oGPU/gpu.h>

#define oGFX_CB_INDEX_PER_FRAME 0
#define oGFX_CB_INDEX_PER_VIEW  1
#define oGFX_CB_INDEX_PER_DRAW  2

#define oGFX_SRV_DIFFUSE_ALPHA  0
#define oGFX_SRV_SPECULAR_GLOSS 1
#define oGFX_SRV_NORMALS_HEIGHT 2

#define oGFX_RTV_GBUF_DEPTH          0
#define oGFX_RTV_GBUF_DIFFUSE_ALPHA  1
#define oGFX_RTV_GBUF_SPECULAR_GLOSS 2
#define oGFX_RTV_GBUF_NORMALS_FLAGS  3

namespace ouro { namespace gfx2 {

enum class pipeline_state
{
	unknown,
	lines,
	count,
};

gpu::pipeline_state_desc get_pipeline_state_desc(const pipeline_state& state);

}}
