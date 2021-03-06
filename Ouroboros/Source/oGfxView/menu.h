// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include <oCore/countof.h>
#include <oMath/gizmo.h>
#include <oGUI/oGUI.h>

#include "camera_control.h"

namespace ouro { namespace ui {

namespace menu { enum value
{
	file,
	edit,
	edit_manip_space,
	view,
	view_camera_control,
	view_style,
	view_state,
	view_render_state,
	help,
	count,
	top_level,
};}

struct menu_hierarchy
{
	menu::value parent;
	menu::value child;
	const char* name;
};

static const menu_hierarchy s_menu_hierarchy[] = 
{
	{ menu::top_level, menu::file,                "&File"           },
	{ menu::top_level, menu::edit,                "&Edit"           },
	{ menu::edit,      menu::edit_manip_space,    "&Manip Space"    },
	{ menu::top_level, menu::view,                "&View"           },
	{ menu::view,      menu::view_camera_control, "&Camera Control" },
	{ menu::view,      menu::view_style,          "Border Style"    },
	{ menu::view,      menu::view_state,          "&Window State"   },
	{ menu::view,      menu::view_render_state,   "&Render State"   },
	{ menu::top_level, menu::help,                "&Help"           },
};
match_array_e(s_menu_hierarchy, menu);

namespace menu_item { enum value
{
	file_load_mesh,
	file_exit,

	edit_manip_space_first,
	edit_manip_space_last = edit_manip_space_first + (int)gizmo::space::count - 1,

	view_camera_control_first,
	view_camera_control_last = view_camera_control_first + (int)camera_control::type::count - 1,

	view_style_first,
	view_style_last = view_style_first + (int)window_style::count - 1,

	view_state_first,
	view_state_last = view_state_first + (int)window_state::count - 1,

	view_render_state_first,
	view_render_state_last = view_render_state_first + (int)gfx::fullscreen_mode::count - 1,

	view_default,
	view_exclusive,

	help_about,
};}

namespace statusbar { enum value
{
	camera_coords,
	mouse_coords,
	default_style_message,
	camera_type,
	fullscreen_type,
};}

}}

