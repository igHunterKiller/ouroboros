// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGfx/bytecode.h>
#include <oCore/stringize.h>
#include "bytecode_util.h"

#include <VSpos.h>
#include <VSpos_nrm_tan.h>
#include <VSpos_uv0.h>
#include <VSpos_uvw.h>
#include <VSpos_col.h>
#include <VSpos_uv0_col.h>
#include <VSpos_nrm_tan_uv0.h>
#include <VSpos_nrm_tan_uvw.h>
#include <VSpos_nrm_tan_col.h>		
#include <VSpos_nrm_tan_uv0_uv1.h>
#include <VSpos_nrm_tan_uv0_col.h>
#include <VSpos_nrm_tan_uvw_col.h>
#include <VSpos_nrm_tan_uv0_uv1_col.h>
#include <VSpos_passthru.h>
#include <VSpos_col_passthru.h>
#include <VSpos_col_prim.h>
#include <VSfullscreen_tri.h>
#include <VSpos_as_uvw.h>

#include <PSblack.h>
#include <PSgray.h>
#include <PSwhite.h>
#include <PSred.h>
#include <PSgreen.h>
#include <PSblue.h>
#include <PSyellow.h>
#include <PSmagenta.h>
#include <PScyan.h>
#include <PSconstant_color.h>
#include <PStexture1d.h>
#include <PStexture1d_array.h>
#include <PStexture2d.h>
#include <PStexture2d_array.h>
#include <PStexture3d.h>
#include <PStexture_cube.h>
#include <PStexture_cube_array.h>
#include <PSvertex_color.h>
#include <PSvertex_color_prim.h>
#include <PStexcoordu.h>
#include <PStexcoordv.h>
#include <PStexcoord.h>
#include <PStexcoord3u.h>
#include <PStexcoord3v.h>
#include <PStexcoord3w.h>
#include <PStexcoord3.h>
#include <GSvertex_normals.h>
#include <GSvertex_tangents.h>

namespace ouro { namespace gfx {

mesh::layout_t layout(const vertex_layout& input)
{
	static const mesh::celement_t ePos(mesh::element_semantic::position, 0, surface::format::r32g32b32_float,    0);
	static const mesh::celement_t eNrm(mesh::element_semantic::normal,   0, surface::format::r32g32b32_float,    0);
	static const mesh::celement_t eTan(mesh::element_semantic::tangent,  0, surface::format::r32g32b32a32_float, 0);
	static const mesh::celement_t eUv0(mesh::element_semantic::texcoord, 0, surface::format::r32g32_float,       0);
	static const mesh::celement_t eUv1(mesh::element_semantic::texcoord, 1, surface::format::r32g32_float,       0);
	static const mesh::celement_t eUvw(mesh::element_semantic::texcoord, 0, surface::format::r32g32b32_float,    0);
	static const mesh::celement_t eColor(mesh::element_semantic::color,  0, surface::format::b8g8r8a8_unorm,     0);

	mesh::layout_t e;
	e.fill(mesh::celement_t());
	switch (input)
	{
		case vertex_layout::none: break;
		case vertex_layout::pos:                     e[0] = ePos;                                                                    break;
		case vertex_layout::pos_nrm_tan:             e[0] = ePos; e[1] = eNrm; e[2] = eTan;                                          break;
		case vertex_layout::pos_uv0:                 e[0] = ePos; e[1] = eUv0;                                                       break;
		case vertex_layout::pos_uvw:                 e[0] = ePos; e[1] = eUvw;                                                       break;
		case vertex_layout::pos_col:                 e[0] = ePos; e[1] = eColor;                                                     break;
		case vertex_layout::pos_uv0_col:             e[0] = ePos; e[1] = eUv0; e[2] = eColor;                                        break;
		case vertex_layout::pos_nrm_tan_uv0:         e[0] = ePos; e[1] = eNrm; e[2] = eTan; e[3] = eUv0;                             break;
		case vertex_layout::pos_nrm_tan_uvw:         e[0] = ePos; e[1] = eNrm; e[2] = eTan; e[3] = eUvw;                             break;
		case vertex_layout::pos_nrm_tan_col:         e[0] = ePos; e[1] = eNrm; e[2] = eTan; e[3] = eColor;                           break;
		case vertex_layout::pos_nrm_tan_uv0_uv1:     e[0] = ePos; e[1] = eNrm; e[2] = eTan; e[3] = eUv0; e[4] = eUv1;                break;
		case vertex_layout::pos_nrm_tan_uv0_col:     e[0] = ePos; e[1] = eNrm; e[2] = eTan; e[3] = eUv0; e[4] = eColor;              break;
		case vertex_layout::pos_nrm_tan_uvw_col:     e[0] = ePos; e[1] = eNrm; e[2] = eTan; e[3] = eUvw; e[4] = eColor;              break;
		case vertex_layout::pos_nrm_tan_uv0_uv1_col: e[0] = ePos; e[1] = eNrm; e[2] = eTan; e[3] = eUv0; e[4] = eUv1; e[5] = eColor; break;
		case vertex_layout::uv0:                     e[0] = eUv0;                                                                    break;
		case vertex_layout::uv0_uv1:                 e[0] = eUv0; e[1] = eUv1;                                                       break;
		case vertex_layout::uvw:                     e[0] = eUvw;                                                                    break;
		case vertex_layout::col:                     e[0] = eColor;                                                                  break;
		case vertex_layout::uv0_col:                 e[0] = eUv0; e[1] = eColor;                                                     break;
		case vertex_layout::uv0_uv1_col:             e[0] = eUv0; e[1] = eUv1; e[2] = eColor;                                        break;
		case vertex_layout::uvw_col:                 e[0] = eUvw; e[1] = eColor;                                                     break;

		default: break;
	}

	return e;
}

oSH_BEGIN(vertex_shader)
{
	oSH_NULL,
	oSH(VSpos),
	oSH(VSpos_nrm_tan),
	oSH(VSpos_uv0),
	oSH(VSpos_uvw),
	oSH(VSpos_col),
	oSH(VSpos_uv0_col),
	oSH(VSpos_nrm_tan_uv0),
	oSH(VSpos_nrm_tan_uvw),
	oSH(VSpos_nrm_tan_col),		
	oSH(VSpos_nrm_tan_uv0_uv1),
	oSH(VSpos_nrm_tan_uv0_col),
	oSH(VSpos_nrm_tan_uvw_col),
	oSH(VSpos_nrm_tan_uv0_uv1_col),
	oSH(VSpos_passthru),
	oSH(VSpos_col_passthru),
	oSH(VSpos_col_prim),
	oSH(VSfullscreen_tri),
	oSH(VSpos_as_uvw),
};
oSH_END(vertex_shader)

const void* sig_bytecode(const vertex_layout& input)
{
	if ((int)input >= (int)vertex_layout::sig_count)
		return nullptr;
	return s_vertex_shader[(int)input].bytecode;
}

oSH_BEGIN(hull_shader)
{
	oSH_NULL,
};
oSH_END(hull_shader)

oSH_BEGIN(domain_shader)
{
	oSH_NULL,
};
oSH_END(domain_shader)

oSH_BEGIN(geometry_shader)
{
	oSH_NULL,
	oSH(GSvertex_normals),
	oSH(GSvertex_tangents),
};
oSH_END(geometry_shader)

oSH_BEGIN(pixel_shader)
{
	oSH_NULL,
	oSH(PSblack),
	oSH(PSgray),
	oSH(PSwhite),
	oSH(PSred),
	oSH(PSgreen),
	oSH(PSblue),
	oSH(PSyellow),
	oSH(PSmagenta),
	oSH(PScyan),
	oSH(PSconstant_color),
	oSH(PSvertex_color_prim),
	oSH(PStexture1d),
	oSH(PStexture1d_array),
	oSH(PStexture2d),
	oSH(PStexture2d_array),
	oSH(PStexture3d),
	oSH(PStexture_cube),
	oSH(PStexture_cube_array),
	oSH(PSvertex_color),
	oSH(PStexcoordu),
	oSH(PStexcoordv),
	oSH(PStexcoord),
	oSH(PStexcoord3u),
	oSH(PStexcoord3v),
	oSH(PStexcoord3w),
	oSH(PStexcoord3),
};
oSH_END(pixel_shader)

oSH_BEGIN(compute_shader)
{
	oSH_NULL,
};
oSH_END(compute_shader)

}

template<> const char* as_string(const gfx::vertex_layout& layout)
{
	const char* s_names[] = 
	{
		"none",
		"pos",
		"pos_nrm_tan",
		"pos_uv0",
		"pos_uvw",
		"pos_col",
		"pos_uv0_col",
		"pos_nrm_tan_uv0",
		"pos_nrm_tan_uvw",
		"pos_nrm_tan_col",
		"pos_nrm_tan_uv0_uv1",
		"pos_nrm_tan_uv0_col",
		"pos_nrm_tan_uvw_col",
		"pos_nrm_tan_uv0_uv1_col",
		"uv0",
		"uv0_uv1",
		"uvw",
		"col",
		"uv0_col",
		"uv0_uv1_col",
		"uvw_col",
	};
	return as_string(layout, s_names);
}

}
