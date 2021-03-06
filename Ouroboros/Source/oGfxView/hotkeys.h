// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include <oBase/input.h>

namespace ouro { namespace ui {

namespace hotkey { enum value
{
	load_mesh,
	toggle_ui_mode,
	default_style,
	toggle_fullscreen,
	toggle_camera_control,
	focus,
	manip_scale,
	manip_rotation,
	manip_translation,
	manip_none,

	render_solid,
	render_wire,
	render_texcoord,
	render_texcoordu,
	render_texcoordv,
};}

static const ouro::hotkey s_hotkeys[] =
{
	{ key::o,     key_modifier::ctrl, hotkey::load_mesh             },
	{ key::f2,    key_modifier::none, hotkey::toggle_ui_mode        },
	{ key::f3,    key_modifier::none, hotkey::default_style         },
	{ key::enter, key_modifier::alt,  hotkey::toggle_fullscreen     },
	{ key::enter, key_modifier::ctrl, hotkey::toggle_camera_control },
	{ key::f,     key_modifier::none, hotkey::focus                 },
	
	{ key::r,     key_modifier::none, hotkey::manip_scale           },
	{ key::e,     key_modifier::none, hotkey::manip_rotation        },
	{ key::w,     key_modifier::none, hotkey::manip_translation     },
	{ key::q,     key_modifier::none, hotkey::manip_none            },

	{ key::z,     key_modifier::none, hotkey::render_solid          },
	{ key::x,     key_modifier::none, hotkey::render_wire           },
	{ key::c,     key_modifier::none, hotkey::render_texcoord       },
	{ key::v,     key_modifier::none, hotkey::render_texcoordu      },
	{ key::b,     key_modifier::none, hotkey::render_texcoordv      },
};

}}
