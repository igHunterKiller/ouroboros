// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "camera_control.h"

#include <oGUI/window.h>
#include <oMath/pov.h>
#include <oSystem/peripherals.h>

namespace ouro {

template<> const char* as_string(const camera_control::type_t& type)
{
	static const char* s_names[] = 
	{
		"Maya",
		"MMO",
	};
	return detail::enum_as(type, s_names);
}

static float3 calc_walk_delta(const keyboard_t& keyboard, bool auto_forward, float walk_speed)
{
	float3 walk = kZero3;

	if (keyboard.down(key::w) || auto_forward) walk.z += 1.0f;
	if (keyboard.down(key::s))                 walk.z -= 1.0f;
	if (keyboard.down(key::d))                 walk.x += 1.0f;
	if (keyboard.down(key::a))                 walk.x -= 1.0f;
	if (keyboard.down(key::space))             walk.y += 1.0f;
	if (keyboard.down(key::z))                 walk.y -= 1.0f;

	float len = length(walk);
	if (!equal(len, 0.0f))
		walk = (walk / len) * walk_speed;

	return walk;
}

bool camera_control::update(const keyboard_t& keyboard, const mouse_t& mouse, window& inout_window, pov_t& inout_pov)
{
	auto state  = arcball::type::none;
	float3 look = kZero3;

	if (enabled_)
	{
		const auto mdn   = mouse.down(mouse_button::middle) || (mouse.down(mouse_button::left)  && mouse.down(mouse_button::right));
		const auto ldn   = mouse.down(mouse_button::left  ) && !mdn;
		const auto rdn   = mouse.down(mouse_button::right ) && !mdn;
		const auto wheel = mouse.wheel_delta();
		const auto x     = mouse.x();
		const auto y     = mouse.y();
		const auto dx    = mouse.x_delta();
		const auto dy    = mouse.y_delta();
		const auto dw    = mouse.wheel_delta();

		// Update speed
		if (wheel)
			walk_speed_ = __max(0.01f, walk_speed_ + 0.001f * wheel);
		
		look = float3(dx, dy, 0.0f);

		switch (type_)
		{
			case type::mmo:
			{
				if (rdn)
				{
					state = arcball::type::look;
					look *= 0.02f;
				}

				float3 walk = calc_walk_delta(keyboard, ldn, walk_speed_);
				arcball_.walk(walk);

				break;
			}

			case type::dcc:
			{
				if (mdn)
				{
					state = arcball::type::pan;

					if (inout_pov.pointer_depth() == 0.0f)
						look *= 0.04f;
					else
						look = float3(inout_pov.unprojected_delta(x - dx, y - dy, x, y), 0.0f);
				}
					
				else if (rdn)
				{
					state = arcball::type::dolly;
					look *= 0.04f;
				}
					
				else if (ldn)
				{
					state = arcball::type::orbit;
					look *= 0.02f;
				}

				else if (dw)
				{
					state = arcball::type::dolly;
					look  = float3(dw * 0.04f, 0.0f, 0.0f);
				}

				break;
			}
		}
	}
	
	auto prior = arcball_.update(state, look);

	// update capture status
	switch (state)
	{
		case arcball::type::orbit:
		case arcball::type::pan:
		case arcball::type::dolly:
			inout_window.capture(mouse_capture::absolute);
			break;
		case arcball::type::look:
			inout_window.client_cursor(mouse_cursor::none);
			inout_window.capture(mouse_capture::relative);
			break;
		case arcball::type::none:
			if (prior != arcball::type::none)
			{
				inout_window.client_cursor(mouse_cursor::arrow);
				inout_window.capture(mouse_capture::none);
			}
			break;
		default:
			break;
	}

	// Update pov's view
	inout_pov.view(arcball_.view());
	
	return enabled_;
}

}