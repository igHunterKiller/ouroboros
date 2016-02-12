// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// common peripheral attributes

#pragma once

namespace ouro {

// _____________________________________________________________________________
// Common

enum class peripheral_status : uint8_t
{
	unknown,
	disabled,
	not_supported,
	not_connected,
	not_ready,
	initializing,
	low_power,
	med_power,
	full_power,

	count,
};

// _____________________________________________________________________________
// Keyboard

enum class key : uint8_t
{
	none,

	// modifier keys
	lctrl, rctrl, lalt, ralt, lshift, rshift, lwin, rwin, app_cycle, app_context,

	// toggle keys
	capslock, scrolllock, numlock,

	// typing keys
	space, backtick, dash, equal_, lbracket, rbracket, backslash, semicolon, apostrophe, comma, period, slash,
	_0, _1, _2, _3, _4, _5, _6, _7, _8, _9,
	a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,

	// numpad keys
	num0, num1, num2, num3, num4, num5, num6, num7, num8, num9, 
	nummul, numadd, numsub, numdecimal, numdiv, 

	// typing control keys
	esc, backspace, tab, enter, ins, del, home, end, pgup, pgdn,

	// system control keys
	f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, 
	pause, sleep, printscreen,

	// directional keys
	left, up, right, down,

	// browser keys
	mail, back, forward, refresh, stop, search, favs,

	// media keys
	media, mute, volup, voldn, prev_track, next_track, stop_track, play_pause_track,

	// misc keys
	app1, app2,

	count,
};

enum class key_modifier : uint8_t
{
	none,
	ctrl,
	alt,
	ctrl_alt,
	shift,
	ctrl_shift,
	alt_shift,
	ctrl_alt_shift,

	count,
};

struct hotkey
{
	key key;
	key_modifier modifier;
	uint16_t id;
};

struct key_event
{
	key key;
	bool down;
};

struct hotkey_event
{
	void* window;
	int16_t id;
};

// _____________________________________________________________________________
// Mouse (assumes Microsoft-style design)

enum class mouse_button : uint8_t
{
	left       = 1<<0,
	middle     = 1<<1,
	right      = 1<<2,
	side1      = 1<<3,
	side2      = 1<<4,
	dbl_left   = 1<<5,
	dbl_middle = 1<<6,
	dbl_right  = 1<<7,

	count      = 8,
};

enum class mouse_capture : uint8_t
{
	// normal mouse movement: mouse outside of client area doesn't trigger events
	none,

	// mouse can move outside the client area but all events go to the window
	// positions are still in client space, though can go outside client limits.
	// It will still be confined by virtual desktop limits.
	absolute,

	// mouse position is frozen and only deltas are valid. This is useful for 
	// fps-style controls.
	relative,

	count,
};

enum class mouse_cursor
{
	none,            // No cursor appears (hidden)
	arrow,           // Default OS arrow
	hand,            // A hand for translating/scrolling
	help,            // A question-mark-like icon
	not_allowed,     // Indicates interaction is forbidden
	wait_foreground, // Input blocked while the operation is occurring
	wait_background, // A performance-degrading action is progressing without blocking user input
	user,            // A user-provided cursor is displayed

	count,
};

struct mouse_event
{
	enum type : uint8_t
	{
		press,
		move,
	};

	struct mouse_button_press
	{
		mouse_button button;
		bool down;
	};

	struct mouse_move
	{
		int16_t x;
		int16_t y;
		int16_t x_delta;
		int16_t y_delta;
		int16_t wheel_delta;
	};

	union
	{
		mouse_button_press pr;
		mouse_move mv;
	};

	type type;
};

// _____________________________________________________________________________
// Controller Pad (assumes PS2 Dual-Shock design)

enum class pad_button : uint16_t
{
	lleft      = 1<<0,
	lup        = 1<<1,
	lright     = 1<<2,
	ldown      = 1<<3,
	rleft      = 1<<4,
	rup        = 1<<5,
	rright     = 1<<6,
	rdown      = 1<<7,
	lshoulder1 = 1<<8,
	lshoulder2 = 1<<9,
	rshoulder1 = 1<<10,
	rshoulder2 = 1<<11,
	lthumb     = 1<<12,
	rthumb     = 1<<13,
	start      = 1<<14,
	select     = 1<<15,

	count      = 16,
};

struct pad_event
{
	enum type : uint8_t
	{
		press,
		move,
	};

	struct pad_button_press
	{
		pad_button button;
		bool down;
	};

	struct pad_move
	{
		float lx;
		float ly;
		float rx;
		float ry;
		float ltrigger;
		float rtrigger;
	};

	union
	{
		pad_button_press pr;
		pad_move mv;
	};

	uint8_t type;
	uint8_t index;
};

// _____________________________________________________________________________
// Skeleton (assumes Microsoft Kinect design)

namespace skeleton_clip { enum flag : uint8_t {

	left   = 1<<0,
	right  = 1<<1,
	top    = 1<<2,
	bottom = 1<<3,
	front  = 1<<4,
	back   = 1<<5,
};}

enum class skeleton_status : uint8_t
{
	update,
	acquired,
	lost,

	count,
};

enum class skeleton_bone : uint8_t
{	
	hip_center,
	spine,
	shoulder_center,
	head,
	shoulder_left,
	elbow_left,
	wrist_left,
	hand_left,
	shoulder_right,
	elbow_right,
	wrist_right,
	hand_right,
	hip_left,
	knee_left,
	ankle_left,
	foot_left,
	hip_right,
	knee_right,
	ankle_right,
	foot_right,
	
	count,
	invalid_bone = count,
};

struct skeleton_t
{
	void* handle;
	skeleton_status status;
	uint8_t index;
	uint16_t unused0;
};

// _____________________________________________________________________________
// Touch

struct touch_t
{
	enum { max_touch = 10 };

	int16_t x[max_touch];
	int16_t y[max_touch];
};

}
