// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// The visible representation of the pointer on the screen and any status 
// changes that feed how that position is reported. Basically the cursor is the 
// part of a typical mouse interaction that does not include mouse buttons.

#pragma once
#include <oCore/peripherals.h>

namespace ouro { namespace cursor {

// if user_cursor is specified it is used for rendering and all lifetime is 
// transfered to the window. This uses system_cursor, set/get_default and
// show to put the cursor in the specified state. If window is specified the 
// window-default cursor is overwritten. If no window is specified the global
// cursor value is set/get.
void set(void* window, const mouse_cursor& c, void* user_cursor = nullptr);
mouse_cursor get(void* window);

// modifies the default cursor associated with the client area of a window
void* get_default(void* window);
void set_default(void* window, void* cursor);

// returns built-in cursors; nullptr for mouse_cursor::none and user_cursor for 
// mouse_cursor::user
void* system_cursor(const mouse_cursor& c, void* user_cursor = nullptr);

// position is client space if a window is specified or virtual desktop space if 
// null
void position(int16_t x, int16_t y, void* window = nullptr);
void position(int16_t* out_x, int16_t* out_y, void* window = nullptr);

void show(bool shown = true);
bool shown();

// ensures the cursor position stays within window's client area
bool clipped(void* window);
void clip(void* window, bool clipped = true);

// even if the window does not have focus the specified window will continue to 
// receive cursor events. captured() returns the window that is captured or 
// nullptr if one is not captured. This does not respect capture-type since to 
// handle type properly requires more context than provided here. When captured
// all events still go to the specified window, no other consideration is 
// handled by this API.
void* captured();
void capture(void* window, int16_t* out_x = nullptr, int16_t* out_y = nullptr);

// primarily useful for fullscreen apps, this moves the cursor offscreen so even
// if the system unhides the cursor, it's still in an inconspicuous spot.
void move_offscreen();

}}
