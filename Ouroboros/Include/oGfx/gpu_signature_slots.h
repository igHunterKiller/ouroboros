// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_gpu_signature_slots_h
#define oGfx_gpu_signature_slots_h

#define oGFX_SRV_DEPTH           33

#define oGFX_SAMPLER_POINT_CLAMP  0
#define oGFX_SAMPLER_POINT_WRAP   1
#define oGFX_SAMPLER_LINEAR_CLAMP 2
#define oGFX_SAMPLER_LINEAR_WRAP  3
#define oGFX_SAMPLER_ANISO_CLAMP  4
#define oGFX_SAMPLER_ANISO_WRAP   5

#define oGFX_CBV_FRAME            0
#define oGFX_CBV_VIEW             1
#define oGFX_CBV_DRAW             2
#define oGFX_CBV_SHADOW           3
#define oGFX_CBV_MISC             4

#define oGFX_CBV_SLOT(x) register(b##x)
#define oGFX_DECLARE_DRAW_CONSTANTS(x) cbuffer cbuffer_draw : oGFX_CBV_SLOT(oGFX_CBV_DRAW) { draw_constants x; }
#define oGFX_DECLARE_MISC_CONSTANTS(x) cbuffer cbuffer_misc : oGFX_CBV_SLOT(oGFX_CBV_MISC) { misc_constants x; }
#define oGFX_DECLARE_PRIM_CONSTANTS(x) cbuffer cbuffer_draw : oGFX_CBV_SLOT(oGFX_CBV_DRAW) { prim_constants x; }
#define oGFX_DECLARE_VIEW_CONSTANTS(x) cbuffer cbuffer_view : oGFX_CBV_SLOT(oGFX_CBV_VIEW) { view_constants x; }

#include <oGfx/draw_constants.h>
#include <oGfx/misc_constants.h>
#include <oGfx/prim_constants.h>
#include <oGfx/view_constants.h>

#ifndef oHLSL
// placeholders
struct frame_constants  { float rand; };
struct shadow_constants { float4x4 matrix; };
#endif
#endif
