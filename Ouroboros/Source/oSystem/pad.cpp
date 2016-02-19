// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/peripherals.h>
#include <oCore/countof.h>
#include <oMath/quantize.h>

#define WIN32_LEAN_AND_MEAN
#include <xinput.h>

namespace ouro {

static peripheral_status to_status(const XINPUT_BATTERY_INFORMATION& inf)
{
	if (inf.BatteryType == BATTERY_TYPE_DISCONNECTED)
		return peripheral_status::not_connected;

	switch (inf.BatteryLevel)
	{
		case BATTERY_LEVEL_LOW:  return peripheral_status::low_power;
		case BATTERY_LEVEL_MEDIUM: return peripheral_status::med_power;
		case BATTERY_LEVEL_FULL: return peripheral_status::full_power;
		default: break;
	}
	
	return peripheral_status::low_power;
}

static uint16_t to_pad_buttons(const XINPUT_GAMEPAD& pad)
{
	static const uint16_t XButtons[] = 
	{
		XINPUT_GAMEPAD_DPAD_LEFT,
		XINPUT_GAMEPAD_DPAD_UP,
		XINPUT_GAMEPAD_DPAD_RIGHT,
		XINPUT_GAMEPAD_DPAD_DOWN,
		XINPUT_GAMEPAD_X,
		XINPUT_GAMEPAD_Y,
		XINPUT_GAMEPAD_B,
		XINPUT_GAMEPAD_A,
		XINPUT_GAMEPAD_LEFT_SHOULDER,
		0,
		XINPUT_GAMEPAD_RIGHT_SHOULDER,
		0,
		XINPUT_GAMEPAD_LEFT_THUMB,
		XINPUT_GAMEPAD_RIGHT_THUMB,
		XINPUT_GAMEPAD_START,
		XINPUT_GAMEPAD_BACK,
	};
	match_array(XButtons, 16);

	const uint16_t wbuttons = pad.wButtons;
	uint16_t btns = 0;

	if (pad.bLeftTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		btns |= (uint16_t)pad_button::lshoulder2;

	if (pad.bRightTrigger >= XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
		btns |= (uint16_t)pad_button::rshoulder2;

	for (uint16_t b = 0; b < countof(XButtons); b++)
		if (wbuttons & XButtons[b])
			btns |= (1 << b);

	return btns;
}

pad_t::pad_t(uint8_t index)
{
	initialize(index);
}

pad_t::~pad_t()
{
	deinitialize();
}

void pad_t::initialize(uint8_t index)
{
	memset(this, 0, sizeof(*this));
	index_ = index;
	status_ = index == (uint8_t)-1 ? peripheral_status::disabled : peripheral_status::initializing;
}

void pad_t::deinitialize()
{
	memset(this, 0, sizeof(*this));
}

void pad_t::update()
{
	static uint8_t max_num = std::min((uint8_t)max_num_pads, (uint8_t)XUSER_MAX_COUNT);

	if (status_ != peripheral_status::disabled && index_ <= max_num)
	{
		XINPUT_STATE st;
		XINPUT_BATTERY_INFORMATION inf;
		DWORD result = XInputGetState(index_, &st);

		if (result == ERROR_DEVICE_NOT_CONNECTED)
		{
			*this = pad_t(index_);
			status_ = peripheral_status::not_connected;
			return;
		}

		else if (result != ERROR_SUCCESS)
			oThrow(std::errc::invalid_argument, "XInputGetState failed with winerror code: %u", result);

#if 0 // 1.4
		result = XInputGetBatteryInformation(i, BATTERY_DEVTYPE_GAMEPAD, &inf);
		if (result != ERROR_SUCCESS)
			oThrow(std::errc::invalid_argument, stringf("XInputGetBatteryInformation failed with winerror code: %u", result));
#else
		inf.BatteryType = BATTERY_TYPE_UNKNOWN;
		inf.BatteryLevel = BATTERY_LEVEL_FULL;
#endif
		
		prev_down_ = down_;
		down_ = to_pad_buttons(st.Gamepad);
		const uint16_t changed = prev_down_ ^ down_;

		prev_status_ = status_;
		status_ = to_status(inf);
		pressed_ = changed & down_;
		released_ = changed & prev_down_;

		float prev_ljoy_x = ljoy_x_;
		float prev_ljoy_y = ljoy_y_;
		float prev_rjoy_x = rjoy_x_;
		float prev_rjoy_y = rjoy_y_;
		float prev_ltrigger = ltrigger_;
		float prev_rtrigger = rtrigger_;

		ljoy_x_ = abs(st.Gamepad.sThumbLX) >= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? s16tof32(st.Gamepad.sThumbLX) : 0;
		ljoy_y_ = abs(st.Gamepad.sThumbLY) >= XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? s16tof32(st.Gamepad.sThumbLY) : 0;
		rjoy_x_ = abs(st.Gamepad.sThumbRX) >= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? s16tof32(st.Gamepad.sThumbRX) : 0;
		rjoy_y_ = abs(st.Gamepad.sThumbRY) >= XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? s16tof32(st.Gamepad.sThumbRY) : 0;
		ltrigger_ = n8tof32(st.Gamepad.bLeftTrigger);
		rtrigger_ = n8tof32(st.Gamepad.bRightTrigger);

		ljoy_x_delta_ = ljoy_x_ - prev_ljoy_x;
		ljoy_y_delta_ = ljoy_y_ - prev_ljoy_y;
		rjoy_x_delta_ = rjoy_x_ - prev_rjoy_x;
		rjoy_y_delta_ = rjoy_y_ - prev_rjoy_y;
		ltrigger_delta_ = ltrigger_ - prev_ltrigger;
		rtrigger_delta_ = rtrigger_ - prev_rtrigger;
	}

	else
		*this = pad_t(index_);
}

void pad_t::reset()
{
	enable(false);
	memset(this, 0, size_t((uint8_t*)&index_ - (uint8_t*)&prev_down_));
}

bool pad_t::trigger_events(void (*trigger)(const pad_event& e, void* user), void* user)
{
	int nevents = 0;
	pad_event e;

	const bool ljoy_steady = equal(ljoy_x_delta_, 0.0f) && equal(ljoy_y_delta_, 0.0f);
	const bool rjoy_steady = equal(rjoy_x_delta_, 0.0f) && equal(rjoy_y_delta_, 0.0f);
	const bool trig_steady = equal(ltrigger_delta_, 0.0f) && equal(rtrigger_delta_, 0.0f);

	if ( !ljoy_steady || !ljoy_steady || !trig_steady)
	{
		e.mv.lx = ljoy_x_;
		e.mv.ly = ljoy_y_;
		e.mv.rx = rjoy_x_;
		e.mv.ry = rjoy_y_;
		e.mv.ltrigger = ltrigger_;
		e.mv.rtrigger = rtrigger_;
		e.type = pad_event::move;
		e.index = index_;
		if (trigger)
			trigger(e, user);

		nevents++;
	}

	const uint16_t ch = changed();

	if (ch)
	{
		e.type = pad_event::press;
		e.index = index_;

		for (uint16_t k = 0, b = 1; b; k++, b <<= 1)
		{
			if (ch & b)
			{
				e.pr.button = (pad_button)b;
				e.pr.down = pressed((pad_button)b);
				if (trigger)
					trigger(e, user);
				nevents++;
			}
		}
	}

	return nevents > 0;
}

}
