// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Common header for Graphical User Interface (GUI) concepts. This should be 
// used as the basis of a cross-platform interface and conversion functions from 
// these generic concepts to platform-specific concepts.

#pragma once
#include <oMath/hlsl.h>
#include <oCore/assert.h>
#include <oCore/color.h>
#include <oString/fixed_string.h>
#include <oBase/input.h>
#include <oCore/macros.h>
#include <array>
#include <functional>

namespace ouro {

namespace alignment
{ enum value {

	// Position is relative to parent's top-left corner
	top_left,

	// Position is centered horizontally and vertically relative to the parent's 
	// top
	top_center,

	// Position is relative to parent's top-right corner minus the child's width
	top_right,

	// Position is centered vertically and horizontally relative to parent's left 
	// corner
	middle_left,
	
	// Position is relative to that which centers the child in the parent's bounds
	middle_center,
	
	// Position is centered vertically and horizontally relative to parent's right 
	// corner minus the child's width
	middle_right,
	
	// Position is relative to parent's bottom-left corner
	bottom_left,

	// Position is centered horizontally and vertically relative to the parent's 
	// bottom minus the child's height
	bottom_center,
	
	// Position is relative to parent's bottom-right corner minus the child's width
	bottom_right,
	
	// Child is sized and positioned to match parent: aspect ration is not 
	// respected. Alignment will be center-middle and respect any offset value.
	fit_parent,

	// Retain aspect ratio and choose the child's largest dimension (width or 
	// height) and leave letterbox (vertical or horizontal) areas. Alignment will 
	// be center-middle and respect any offset value.
	fit_largest_axis,

	// Retain aspect ration and choose the child's smallest dimension (width or 
	// height) and crop any overflow. (cropping of the result rectangle is 
	// dependent on a separate cropping flag.
	fit_smallest_axis,

	count,

};}

namespace window_state
{	enum value {

	// A GUI window is a rectangular client area with an operating system-specific
	// border. It can exist in one of the several states listed below.
	
	// Window does not exist, or when specifying a new state, use this to indicate 
	// no-change.
	invalid,
	
	// Window is invisible.
	hidden,

	// Window is reduces to iconic or taskbar size. When setting this size
	// and position are ignored but style is preserved.
	minimized,
	
	// Window is in normal sub-screen-size mode. When setting this state size,  
	// position and style are respected.
	restored,

	// Window takes up the entire screen but still has borders. When setting this 
	// state size and position are ignored but style is preserved.
	maximized, 

	// Window borders are removed and the client area fills the whole screen but 
	// does not do special direct-access or HW-syncing - it still behaves like a 
	// window thus enabling fast application switching or multi-full-screen
	// support.
	fullscreen,
	
	count,

};}

namespace window_style
{	enum value {

	// Use this value for "noop" or "previous" value.
	default_style,

	// There is no border around the client area and the window remains a top-
	// level window. This is the best for splash screens and fullscreen modes.
	borderless,

	// There is a border but closing the window from the decoration is not allowed
	dialog,

	// There is a border but no user resize is allowed
	fixed,

	// Same as fixed with a menu
	fixed_with_menu,

	// Same as fixed with a status bar
	fixed_with_statusbar,

	// Same as fixed with a menu and status bar
	fixed_with_menu_and_statusbar,

	// There is a border and user can resize window
	sizable,

	// Same as sizable with a menu
	sizable_with_menu,

	// Same as sizable with a status bar
	sizable_with_statusbar,

	// Same as sizable with a menu and a status bar
	sizable_with_menu_and_statusbar,

	count,

};}

namespace window_sort_order
{	enum value {

	// normal/default behavior
	sorted,

	// "normal" always-on-top
	always_on_top,

	// Meant for final shipping, this fights hard to ensure any Windows popups or
	// even the taskbar stays hidden.
	always_on_top_with_focus,

	count,

};}

namespace capture_state
{	enum value {

	// normal mouse movement: mouse outside of window doesn't trigger events
	none,

	// the mouse cursor is clipped to the client area of the window
	client,
	
	// mouse can move outside the client area but all events go to the window
	unconstrained,

	// after each mouse move the cursor is set to 0,0 thus only deltas are valid
	cursor_recentered,

	count,

};}

namespace control_type
{	enum value {

	unknown,
	group,
	button,
	checkbox,
	radio,
	label,
	label_centered,
	hyperlabel, // supports multiple markups of <a href="<somelink>">MyLink</a> or <a id="someID">MyID</a>. There can be multiple in one string.
	label_selectable,
	icon,
	textbox,
	textbox_scrollable,
	floatbox, // textbox that only allows floating point (single-precision) numbers to be entered. Specify oDEFAULT for Size values to use icon's values.
	floatbox_spinner,
	combobox, // Supports specifying contents all at once, delimited by '|'. i.e. "Text1|Text2|Text3"
	combotextbox, // Supports specifying contents all at once, delimited by '|'. i.e. "Text1|Text2|Text3"
	progressbar,
	progressbar_unknown, // (marquee)
	tab, // Supports specifying contents all at once, delimited by '|'. i.e. "Text1|Text2|Text3"
	slider,
	slider_selectable, // displays a portion of the slider as selected
	slider_with_ticks, // Same as Slider but tick marks can be added
	listbox, // Supports specifying contents all at once, delimited by '|'. i.e. "Text1|Text2|Text3"
	count,

};}

namespace border_style
{	enum value {

	sunken,
	flat,
	raised,

	count,

};}

namespace event_type
{	enum value {

	// An event is something that the window issues to client code. Events can be
	// triggered as a side-effect of other actions. Rarely does client code have
	// direct control over events. All events should be downcast to AsShape() 
	// unless described otherwise below.

	// Called when a timer is triggered. Use timers only for infrequent one-shot
	// actions. Do not use this mechanism for consistent updates such as ring
	// buffer monitors or for main loop execution or swap buffer presentation.
	// Use AsTimer() on the event.
	timer,

	activated, // called just after a window comes to be in focus
	deactivated, // called just after a window is no longer in focus

	// called before during window or control creation. This should throw an 
	// exception if there is a failure. It will be caught in the window's factory
	// function and made into an API-consistent error message or rethrown as 
	// appropriate.
	creating, 
	
	// called when the window wants to redraw itself. NOTE: On Windows the user 
	// must call BeginPaint/EndPaint on the event's hSource as documented in 
	// WM_PAINT.
	paint,
	display_changed, // called when the desktop/screens change
	moving, // called just before a window moves
	moved, // called just after a window moves
	sizing, // called just before a window's client area changes size
	sized, // called just after a window's client area changes size
	
	// called before the window is closed. Really this is where client code should 
	// decide whether or not to exit the window's message loop. ouro::event_type::closed is 
	// too late. All control of the message loop is in client code, so if there
	// is nothing done in ouro::event_type::closing, then the default behavior is to leave the 
	// window as-is, no closing is actually done.
	closing, 
	
	// called after a commitment to closing the window has been made.
	closed,
	
	// called when a request to become fullscreen is made. Hook can return false 
	// on failure.
	to_fullscreen,
	
	// called when a request to go from fullscreen to windowed is made
	from_fullscreen,

	// Sent if a window had input capture, but something else in the system caused 
	// it to lose capture
	lost_capture, 
	
	// sent if a window gets files drag-dropped on it. Use AsDrop() on event.
	drop_files,

	// Sent when an input device's status has changed. Use AsInputDevice() on 
	// event.
	input_device_changed,

	// The user can trigger a custom event with a user-specified sub-type. 
	// Use AsCustom() on event.
	custom_event,

	count,

};}

oDECLARE_HANDLE(draw_context_handle);
oDECLARE_HANDLE(window_handle);
oDECLARE_HANDLE(menu_handle);
oDECLARE_DERIVED_HANDLE(window_handle, statusbar_handle);
oDECLARE_DERIVED_HANDLE(window_handle, control_handle);
oDECLARE_HANDLE(font_handle);
oDECLARE_HANDLE(icon_handle);
oDECLARE_HANDLE(cursor_handle);

struct window_shape
{
	window_shape()
		: state(window_state::invalid)
		, style(window_style::default_style)
		, client_position(oDEFAULT, oDEFAULT)
		, client_size(oDEFAULT, oDEFAULT)
	{}

	// Minimize and maximize will override/ignore client_position and client_size 
	// values. Use window_state::invalid to indicate the state should remain 
	// untouched (only respect client_position and client_size values).
	window_state::value state;

	// Use window_style::default_style to indicate that the style should remain 
	// untouched. Changing style will not affect client size/position.
	window_style::value style;

	// This always refers to non-minimized, non-maximized states. oDEFAULT values 
	// imply "use whatever was there before". For example if the state is set to 
	// maximized and some non-default value is applied to client_size then the 
	// next time the state is set to restored the window will have that new size. 
	int2 client_position;
	int2 client_size;
};

struct control_info
{
	control_info()
		: parent(nullptr)
		, font(nullptr)
		, type(control_type::unknown)
		, text(nullptr)
		, position(oDEFAULT, oDEFAULT)
		, size(oDEFAULT, oDEFAULT)
		, id(0xffff)
		, starts_new_group(false)
	{}

	// All controls must have a parent. It can be the top-level window or other
	// controls.
	window_handle parent;

	// If nullptr is specified then the system default font will be used.
	font_handle font;

	// Type of control to create
	control_type::value type;
	
	// Any item that contains a list of strings can set this to be a '|'-delimited 
	// set of strings to immediately populate all items. ("Item1|Item2|Item3").
	union
	{
		const char* text;
		icon_handle icon;
	};

	int2 position;
	int2 size;
	unsigned short id;
	bool starts_new_group;
};

struct font_info
{
	font_info()
		: name("Tahoma")
		, point_size(10.0f)
		, bold(false)
		, italic(false)
		, underline(false)
		, strikeout(false)
		, antialiased(true)
	{}

	sstring name;
	float point_size;
	bool bold;
	bool italic;
	bool underline;
	bool strikeout;
	bool antialiased;
};

struct text_info
{
	text_info()
		: position(int2(oDEFAULT, oDEFAULT))
		, size(int2(oDEFAULT, oDEFAULT))
		, alignment(alignment::top_left)
		, argb_foreground(color::white)
		, argb_background(color::null)
		, argb_shadow(color::null)
		, shadow_offset(int2(2,2))
		, single_line(false)
	{}

	// Position and size define a rectangle in which text is drawn. Alignment 
	// provides addition offsetting within that rectangle. So to center text on
	// the screen the position would be 0,0 and size would be the resolution of 
	// the screen and alignment would be middle-center.
	int2 position;
	int2 size;
	alignment::value alignment;
	// Any non-1.0 (non-0xff) alpha will be not-drawn
	uint32_t argb_foreground;
	uint32_t argb_background;
	uint32_t argb_shadow;
	int2 shadow_offset;
	bool single_line;
};

// no ctor so static init works
struct basic_menu_item_info
{
	const char* text;
	bool enabled;
	bool checked;
};

struct menu_item_info : basic_menu_item_info
{
	menu_item_info() { text = ""; enabled = true; checked = false; }
};

// Invalid, hidden and minimized windows are not visible
inline bool is_visible(const window_state::value& _State) { return _State >= window_state::restored; }

inline bool has_statusbar(const window_style::value& _Style)
{
	switch (_Style)
	{
		case window_style::fixed_with_statusbar:
		case window_style::fixed_with_menu_and_statusbar:
		case window_style::sizable_with_statusbar:
		case window_style::sizable_with_menu_and_statusbar: return true;
		default: break;
	}
	return false;
}

inline bool has_menu(const window_style::value& _Style)
{
	switch (_Style)
	{
		case window_style::fixed_with_menu:
		case window_style::fixed_with_menu_and_statusbar:
		case window_style::sizable_with_menu:
		case window_style::sizable_with_menu_and_statusbar: return true;
		default: break;
	}
	return false;
}

inline bool is_shape_event(const event_type::value& _Event)
{
	switch (_Event)
	{
		case event_type::creating: case event_type::timer: case event_type::drop_files: 
		case event_type::input_device_changed: case event_type::custom_event: return false;
		default: break;
	}
	return true;
}

// _____________________________________________________________________________
// Rectangle utils

// Replaces any oDEFAULT values with the specified default value.
inline int2 resolve_default(const int2& _Size, const int2& _DefaultSize) { int2 result(_Size); if (result.x == oDEFAULT) result.x = _DefaultSize.x; if (result.y == oDEFAULT) result.y = _DefaultSize.y; return result; }

// Positions a child rectangle "inside" (parent can be smaller than the child)
// the specified parent according to the specified alignment and clipping. In 
// non-fit alignments, position will be respected as an offset from the anchor
// calculated for the specified alignment. For example, if a rect is right-
// aligned, and there is a position of x=-10, then the rectangle is inset an
// additional 10 units. oDEFAULT for position most often evaluates to a zero 
// offset.
int4 resolve_rect(const int4& _Parent, const int4& _UnadjustedChild, alignment::value _Alignment, bool _Clip);
inline int4 resolve_rect(const int4& _Parent, const int2& _UnadjustedChildPosition, const int2& _UnadjustedChildSize, alignment::value _Alignment, bool _Clip) { int4 r; r.xy() = resolve_default(_UnadjustedChildPosition, int2(0,0)); r.zw() = r.xy() + _UnadjustedChildSize; return resolve_rect(_Parent, r, _Alignment, _Clip); }

}
