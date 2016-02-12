// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Generic structures for dealing with human input to a computer.

#pragma once
#include <oCore/peripherals.h>
#include <functional>
#include <cstdint>

namespace ouro {

struct menu_t
{
	void* window;
	int16_t id;
};

enum class control_status : uint8_t
{
	activated,
	deactivated,
	selection_changing,
	selection_changed,

	count,
};

struct control_t
{
	void* window;
	uint32_t native_status; // in case status doesn't cover it
	int16_t id;
	control_status status;
};

enum class input_type : uint8_t
{
	unknown,
	keypress,
	mouse,
	pad,
	skeleton,
	voice,
	touch,
	hotkey,
	menu,
	control,

	count,
};

struct input_t
{
	input_type type;
	peripheral_status status;
	uint16_t unused0;
	uint32_t timestamp_ms;
	
	union
	{
		control_t control;
		mouse_event mouse;
		hotkey_event hotkey;
		key_event keypress;
		menu_t menu;
		pad_event pad;
		skeleton_t skeleton;
		touch_t touch;
	};
};

typedef std::function<void(const input_t& input)> input_hook;

}
