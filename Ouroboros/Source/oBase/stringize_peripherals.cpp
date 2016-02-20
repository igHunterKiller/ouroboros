// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/peripherals.h>
#include <oString/stringize.h>

namespace ouro {

template<> const char* as_string(const peripheral_status& s)
{
	static const char* s_names[] = 
	{
		"unknown",
		"disabled",
		"not_supported",
		"not_connected",
		"not_ready",
		"initializing",
		"low_power",
		"med_power",
		"full_power",
	};
	return detail::enum_as(s, s_names);
}

oDEFINE_TO_FROM_STRING(peripheral_status);

template<> const char* as_string(const key& k)
{
	static const char* s_names[] = 
	{
		"none",
		"lctrl",
		"rctrl",
		"lalt",
		"ralt",
		"lshift",
		"rshift",
		"lwin",
		"rwin",
		"app_cycle",
		"app_context",
		"capslock",
		"scrolllock",
		"numlock",
		"space",
		"backtick",
		"dash",
		"equal",
		"lbracket",
		"rbracket",
		"backslash",
		"semicolon",
		"apostrophe",
		"comma",
		"period",
		"slash",
		"0",
		"1",
		"2",
		"3",
		"4",
		"5",
		"6",
		"7",
		"8",
		"9",
		"a",
		"b",
		"c",
		"d",
		"e",
		"f",
		"g",
		"h",
		"i",
		"j",
		"k",
		"l",
		"m",
		"n",
		"o",
		"p",
		"q",
		"r",
		"s",
		"t",
		"u",
		"v",
		"w",
		"x",
		"y",
		"z",
		"num0",
		"num1",
		"num2",
		"num3",
		"num4",
		"num5",
		"num6",
		"num7",
		"num8",
		"num9",
		"nummul",
		"numadd",
		"numsub",
		"numdecimal",
		"numdiv",
		"esc",
		"backspace",
		"tab",
		"enter",
		"ins",
		"del",
		"home",
		"end",
		"pgup",
		"pgdn",
		"f1",
		"f2",
		"f3",
		"f4",
		"f5",
		"f6",
		"f7",
		"f8",
		"f9",
		"f10",
		"f11",
		"f12",
		"f13",
		"f14",
		"f15",
		"f16",
		"pause",
		"sleep",
		"printscreen",
		"left",
		"up",
		"right",
		"down",
		"mail",
		"back",
		"forward",
		"refresh",
		"stop",
		"search",
		"favs",
		"media",
		"mute",
		"volup",
		"voldn",
		"prev_track",
		"next_track",
		"stop_track",
		"play_pause_track",
		"app1",
		"app2",
	};
	return detail::enum_as(k, s_names);
}

oDEFINE_TO_FROM_STRING(key);

template<> const char* as_string(const key_modifier& m)
{
	static const char* s_names[] = 
	{
		"none",
		"ctrl",
		"alt",
		"ctrl_alt",
		"shift",
		"ctrl_shift",
		"alt_shift",
		"ctrl_alt_shift",
	};
	return detail::enum_as(m, s_names);
}

oDEFINE_TO_FROM_STRING(key_modifier);

template<> const char* as_string(const mouse_button& b)
{
	static const char* s_names[] = 
	{
		"left",
		"middle",
		"right",
		"side1",
		"side2",
		"dbl_left",
		"dbl_middle",
		"dbl_right",
	};
	return detail::enum_as(b, s_names);
}

oDEFINE_TO_FROM_STRING(mouse_button);

template<> const char* as_string(const mouse_capture& c)
{
	static const char* s_names[] = 
	{
		"none",
		"absolute",
		"relative",
	};
	return detail::enum_as(c, s_names);
}

oDEFINE_TO_FROM_STRING(mouse_capture);

template<> const char* as_string(const mouse_cursor& c)
{
	static const char* s_names[] = 
	{
		"none",
		"arrow",
		"hand",
		"help",
		"not_allowed",
		"wait_foreground",
		"wait_background",
		"user",
	};
	return detail::enum_as(c, s_names);
}

oDEFINE_TO_FROM_STRING(mouse_cursor);

template<> const char* as_string(const pad_button& b)
{
	static const char* s_names[] =
	{
		"lleft",
		"lup",
		"lright",
		"ldown",
		"rleft",
		"rup",
		"rright",
		"rdown",
		"lshoulder1",
		"lshoulder2",
		"rshoulder1",
		"rshoulder2",
		"lthumb",
		"rthumb",
		"start",
		"select",
	};
	return detail::enum_as(b, s_names);
}

oDEFINE_TO_FROM_STRING(pad_button);

}
