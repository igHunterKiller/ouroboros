// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include <oMath/arcball.h>
#include <oMath/hlslx.h>

namespace ouro {

class window;
class keyboard_t;
class mouse_t;
class pov_t;

class camera_control
{
public:
	camera_control()
		: walk_speed_(1.0f)
		, enabled_(true)
		, type_(type::dcc)
	{}

	enum class type : uint8_t
	{
		dcc, // orbit/pan/dolly
		mmo, // wasd with look-around
		count,
	};

	typedef enum class type type_t;

	void type(const type_t& t) { type_ = t; }
	type_t type() const { return type_; }

	// window state management is always handled, but view modification is not done when disabled
	// recommendation: attach enabled state to 'alt down' to emulate maya behavior
	void enable(bool enabled = true) { enabled_ = enabled; }
	bool enabled() const { return enabled_; }

	void focus(float fovy_radians, const float4& sphere) { arcball_.focus(fovy_radians, sphere); }
	void focus(const float3& eye, const float3& at) { arcball_.focus(eye, at); }

	void reset(){ arcball_.update(arcball::type::none, kZero3); }

	// returns true if the view was changed
	bool update(const keyboard_t& keyboard, const mouse_t& mouse, window& inout_window, pov_t& inout_pov);

private:
	arcball arcball_;
	float walk_speed_;
	bool enabled_;
	type_t type_;
};

}
