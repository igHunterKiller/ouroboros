// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Precompiled shader bytecode. All shaders expect draw_constants and oGPU's 
// sampler state-by-index.

#pragma once

#include <cstdint>
#include <oMesh/element.h>

// _____________________________________________________________________________
// Shader enumerations

namespace ouro { namespace gfx { 

enum class vertex_layout : uint8_t
{
	none,
	
	// == Signature layouts ==
	pos,
	pos_nrm_tan,
	pos_uv0,
	pos_uvw,
	pos_col,
	pos_uv0_col,
	pos_nrm_tan_uv0,
	pos_nrm_tan_uvw,
	pos_nrm_tan_col,
	pos_nrm_tan_uv0_uv1,
	pos_nrm_tan_uv0_col,
	pos_nrm_tan_uvw_col,
	pos_nrm_tan_uv0_uv1_col,
	sig_count,
	
	// == Secondary layouts ==
	uv0 = sig_count,
	uv0_uv1,
	uvw,
	col,
	uv0_col,
	uv0_uv1_col,
	uvw_col,

	count,
};

enum class vertex_shader : uint8_t
{
	none,

	// == Layout Shaders == 
	// (must match signature vertex_layout values above)
	// (all take in their specific VTX type and return INTcommon)
	pos,
	pos_nrm_tan,
	pos_uv0,
	pos_uvw,
	pos_col,
	pos_uv0_col,
	pos_nrm_tan_uv0,
	pos_nrm_tan_uvw,
	pos_nrm_tan_col,		
	pos_nrm_tan_uv0_uv1,
	pos_nrm_tan_uv0_col,
	pos_nrm_tan_uvw_col,
	pos_nrm_tan_uv0_uv1_col,

  pos_passthru,            // INTpos position is passed through untransformed 
  pos_col_passthru,        // INTpos_col position is passed through untransformed with vertex color
  pos_col_prim,            // INTpos_col position is transformed by draw_constants
  fullscreen_tri,          // INTpos_uv0 alternative for a fullscreen quad, no VB required, returns INTpos_uv0 in screen coords
  pos_as_uvw,              // INTcommon local-space position xyz in texcoord uvw (for simple cube map testing)

	count,
};

enum class hull_shader : uint8_t
{
	none,

	count,
};

enum class domain_shader : uint8_t
{
	none,

	count,
};

enum class geometry_shader : uint8_t
{
	none,
	vertex_normals,  // in: INTcommon, out: INTpos_col for rendering lines
	vertex_tangents, // in: INTcommon, out: INTpos_col for rendering lines

	count,
};

enum class pixel_shader : uint8_t
{
	none,

	// Solid colors (requires only SSposition)
	black,
	gray,
	white,
	red,
	green,
	blue,
	yellow,
	magenta,
	cyan,
	constant_color, // from draw_constants

	vertex_color_prim, // In: INTpos_col

	// Simple interpolants (texture expected in slot 0, In: INTcommon)
	texture1d,
	texture1d_array,
	texture2d,
	texture2d_array,
	texture3d,
	texture_cube,
	texture_cube_array,
	vertex_color,
	texcoordu,
	texcoordv,
	texcoord,
	texcoord3u,
	texcoord3v,
	texcoord3w,
	texcoord3,

	count,
};

enum class compute_shader : uint8_t
{
	none,

	count,
};

// _____________________________________________________________________________
// Bytecode accessors

// returns the layout of input
mesh::layout_t layout(const vertex_layout& input);

// returns noop vertex shader byte code with the same input signature as input.
const void* sig_bytecode(const vertex_layout& input);
const void* bytecode(const vertex_shader& shader);
const void* bytecode(const hull_shader& shader);
const void* bytecode(const domain_shader& shader);
const void* bytecode(const geometry_shader& shader);
const void* bytecode(const pixel_shader& shader);
const void* bytecode(const compute_shader& shader);

}}
