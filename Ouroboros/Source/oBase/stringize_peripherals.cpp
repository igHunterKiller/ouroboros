// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/peripherals.h>
#include <oString/stringize.h>

namespace ouro {

template<> const char* as_string<peripheral_status>(const peripheral_status& s)
{
	switch (s)
	{
		case peripheral_status::unknown: return "unknown";
		case peripheral_status::disabled: return "disabled";
		case peripheral_status::not_supported: return "not_supported";
		case peripheral_status::not_connected: return "not_connected";
		case peripheral_status::not_ready: return "not_ready";
		case peripheral_status::initializing: return "initializing";
		case peripheral_status::low_power: return "low_power";
		case peripheral_status::med_power: return "med_power";
		case peripheral_status::full_power: return "full_power";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(peripheral_status);

template<> const char* as_string<key>(const key& k)
{
	switch (k)
	{
		case key::none: return "none";
		case key::lctrl: return "lctrl";
		case key::rctrl: return "rctrl";
		case key::lalt: return "lalt";
		case key::ralt: return "ralt";
		case key::lshift: return "lshift";
		case key::rshift: return "rshift";
		case key::lwin: return "lwin";
		case key::rwin: return "rwin";
		case key::app_cycle: return "app_cycle";
		case key::app_context: return "app_context";
		case key::capslock: return "capslock";
		case key::scrolllock: return "scrolllock";
		case key::numlock: return "numlock";
		case key::space: return "space";
		case key::backtick: return "backtick";
		case key::dash: return "dash";
		case key::equal_: return "equal";
		case key::lbracket: return "lbracket";
		case key::rbracket: return "rbracket";
		case key::backslash: return "backslash";
		case key::semicolon: return "semicolon";
		case key::apostrophe: return "apostrophe";
		case key::comma: return "comma";
		case key::period: return "period";
		case key::slash: return "slash";
		case key::_0: return "0";
		case key::_1: return "1";
		case key::_2: return "2";
		case key::_3: return "3";
		case key::_4: return "4";
		case key::_5: return "5";
		case key::_6: return "6";
		case key::_7: return "7";
		case key::_8: return "8";
		case key::_9: return "9";
		case key::a: return "a";
		case key::b: return "b";
		case key::c: return "c";
		case key::d: return "d";
		case key::e: return "e";
		case key::f: return "f";
		case key::g: return "g";
		case key::h: return "h";
		case key::i: return "i";
		case key::j: return "j";
		case key::k: return "k";
		case key::l: return "l";
		case key::m: return "m";
		case key::n: return "n";
		case key::o: return "o";
		case key::p: return "p";
		case key::q: return "q";
		case key::r: return "r";
		case key::s: return "s";
		case key::t: return "t";
		case key::u: return "u";
		case key::v: return "v";
		case key::w: return "w";
		case key::x: return "x";
		case key::y: return "y";
		case key::z: return "z";
		case key::num0: return "num0";
		case key::num1: return "num1";
		case key::num2: return "num2";
		case key::num3: return "num3";
		case key::num4: return "num4";
		case key::num5: return "num5";
		case key::num6: return "num6";
		case key::num7: return "num7";
		case key::num8: return "num8";
		case key::num9: return "num9";
		case key::nummul: return "nummul";
		case key::numadd: return "numadd";
		case key::numsub: return "numsub";
		case key::numdecimal: return "numdecimal";
		case key::numdiv: return "numdiv";
		case key::esc: return "esc";
		case key::backspace: return "backspace";
		case key::tab: return "tab";
		case key::enter: return "enter";
		case key::ins: return "ins";
		case key::del: return "del";
		case key::home: return "home";
		case key::end: return "end";
		case key::pgup: return "pgup";
		case key::pgdn: return "pgdn";
		case key::f1: return "f1";
		case key::f2: return "f2";
		case key::f3: return "f3";
		case key::f4: return "f4";
		case key::f5: return "f5";
		case key::f6: return "f6";
		case key::f7: return "f7";
		case key::f8: return "f8";
		case key::f9: return "f9";
		case key::f10: return "f10";
		case key::f11: return "f11";
		case key::f12: return "f12";
		case key::f13: return "f13";
		case key::f14: return "f14";
		case key::f15: return "f15";
		case key::f16: return "f16";
		case key::pause: return "pause";
		case key::sleep: return "sleep";
		case key::printscreen: return "printscreen";
		case key::left: return "left";
		case key::up: return "up";
		case key::right: return "right";
		case key::down: return "down";
		case key::mail: return "mail";
		case key::back: return "back";
		case key::forward: return "forward";
		case key::refresh: return "refresh";
		case key::stop: return "stop";
		case key::search: return "search";
		case key::favs: return "favs";
		case key::media: return "media";
		case key::mute: return "mute";
		case key::volup: return "volup";
		case key::voldn: return "voldn";
		case key::prev_track: return "prev_track";
		case key::next_track: return "next_track";
		case key::stop_track: return "stop_track";
		case key::play_pause_track: return "play_pause_track";
		case key::app1: return "app1";
		case key::app2: return "app2";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(key);

template<> const char* as_string<key_modifier>(const key_modifier& m)
{
	switch (m)
	{
		case key_modifier::none: return "none";
		case key_modifier::ctrl: return "ctrl";
		case key_modifier::alt: return "alt";
		case key_modifier::ctrl_alt: return "ctrl_alt";
		case key_modifier::shift: return "shift";
		case key_modifier::ctrl_shift: return "ctrl_shift";
		case key_modifier::alt_shift: return "alt_shift";
		case key_modifier::ctrl_alt_shift: return "ctrl_alt_shift";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(key_modifier);

template<> const char* as_string<mouse_button>(const mouse_button& b)
{
	switch (b)
	{
		case mouse_button::left: return "left";
		case mouse_button::middle: return "middle";
		case mouse_button::right: return "right";
		case mouse_button::side1: return "side1";
		case mouse_button::side2: return "side2";
		case mouse_button::dbl_left: return "dbl_left";
		case mouse_button::dbl_middle: "dbl_middle";
		case mouse_button::dbl_right: return "dbl_right";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(mouse_button);

template<> const char* as_string<mouse_capture>(const mouse_capture& c)
{
	switch (c)
	{
		case mouse_capture::none: return "none";
		case mouse_capture::absolute: return "absolute";
		case mouse_capture::relative: return "relative";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(mouse_capture);

template<> const char* as_string<mouse_cursor>(const mouse_cursor& c)
{
	switch (c)
	{
		case mouse_cursor::none: return "none";
		case mouse_cursor::arrow: return "arrow";
		case mouse_cursor::hand: return "hand";
		case mouse_cursor::help: return "help";
		case mouse_cursor::not_allowed: return "not_allowed";
		case mouse_cursor::wait_foreground: return "wait_foreground";
		case mouse_cursor::wait_background: return "wait_background";
		case mouse_cursor::user: return "user";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(mouse_cursor);

template<> const char* as_string<pad_button>(const pad_button& b)
{
	switch (b)
	{
		case pad_button::lleft: return "lleft";
		case pad_button::lup: return "lup";
		case pad_button::lright: return "lright";
		case pad_button::ldown: return "ldown";
		case pad_button::rleft: return "rleft";
		case pad_button::rup: return "rup";
		case pad_button::rright: return "rright";
		case pad_button::rdown: return "rdown";
		case pad_button::lshoulder1: return "lshoulder1";
		case pad_button::lshoulder2: return "lshoulder2";
		case pad_button::rshoulder1: return "rshoulder1";
		case pad_button::rshoulder2: return "rshoulder2";
		case pad_button::lthumb: return "lthumb";
		case pad_button::rthumb: return "rthumb";
		case pad_button::start: return "start";
		case pad_button::select: return "select";
		default: break;
	}
	return "?";
}

oDEFINE_TO_FROM_STRING(pad_button);

}
