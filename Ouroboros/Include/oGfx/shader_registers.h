// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// This header is compiled both by HLSL and C++. It describes the register 
// indices for oGfx-level resources.

#ifndef oHLSL
	#pragma once
#endif
#ifndef oGfx_shader_registers_h
#define oGfx_shader_registers_h

#define oGFX_CONSTANT_BUFFER(slot) register(b##slot)
#define oGFX_RESOURCE_BUFFER(slot) register(t##slot)

#define oGFX_USER_CONSTANTS0 0
#define oGFX_USER_CONSTANTS1 1
#define oGFX_USER_CONSTANTS2 2
#define oGFX_USER_CONSTANTS3 3
#define oGFX_DRAW_CONSTANTS 6
#define oGFX_VIEW_CONSTANTS 7

#define oGFX_USER_RESOURCE0 0
#define oGFX_USER_resource 1
#define oGFX_USER_RESOURCE2 2
#define oGFX_USER_RESOURCE3 3
#define oGFX_GBUFFER_SPECULAR_GLOSS 28
#define oGFX_GBUFFER_DIFFUSE_EMISSIVE 29
#define oGFX_GBUFFER_NORMALS_EXTRA 30
#define oGFX_DEPTH 31

#endif
