// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// State management for common peripherals

#pragma once
#include <oCore/peripherals.h>
#include <cstdint>

namespace ouro {

// _____________________________________________________________________________
// Keyboard

class keyboard_t
{
public:
	typedef void* hotkeyset_t;

	keyboard_t();
	~keyboard_t();

	void initialize();
	void deinitialize();
	
	// call this once per frame to analyze events received since last update call
	void update();

	// call this on lost focus or other major event to clear state
	void reset();

	// call this from an event generator and then update once all events have been
	// triggered
	void trigger(const key_event& e);
	
	inline peripheral_status status() const { return status_; }

	inline void enable(bool en) { status_ = en ? (status_ == peripheral_status::disabled ? peripheral_status::not_ready : status_) : peripheral_status::disabled; }
	inline bool enabled() const { return status_ != peripheral_status::disabled; }

	inline bool down(const key& k) const { return flagged(cur_dn, k); }
	inline bool pressed(const key& k) const { return flagged(cur_pr, k); }
	inline bool released(const key& k) const { return flagged(cur_re, k); }

	// hotkey support: create palettes of hotkeys and activation sets when appropriate
	static hotkeyset_t new_hotkeyset(const hotkey* hotkeys, size_t num_hotkeys);
	static void del_hotkeyset(hotkeyset_t hotkeyset);
	static uint32_t decode_hotkeyset(hotkeyset_t hotkeyset, hotkey* hotkeys, size_t max_hotkeys);

	template<size_t size> static hotkeyset_t new_hotkeyset(const hotkey (&hotkeys)[size]) { return new_hotkeyset(hotkeys, size); }
	template<size_t size> static uint32_t decode_hotkeyset(hotkeyset_t hotkeyset, hotkey (&hotkeys)[size]) { return decode_hotkeyset(hotkeyset, hotkeys, size); }

#if defined(_WIN32)
	static void send(void* hwnd, const key& key, bool down, int16_t x, int16_t y);
	static void send(void* hwnd, const char* msg);
#endif

private:
	inline void index_and_mask(const key& k, int* out_index, uint64_t* out_mask) const { if ((uint8_t)k >= 64) { *out_index = 1; *out_mask = 1ull<<((uint8_t)k-64); } else { *out_index = 0; *out_mask = 1ull << (uint8_t)k; } }
	inline bool flagged(const uint64_t field[2], const key& k) const { int i; uint64_t m; index_and_mask(k, &i, &m); return !!(field[i] & m); }

	uint64_t msg_dn[2];
	uint64_t msg_up[2];
	uint64_t cur_dn[2];
	uint64_t cur_pr[2];
	uint64_t cur_re[2];
	peripheral_status status_;
};

// _____________________________________________________________________________
// Hotkeys

class hotkeyset_t
{
public:
	hotkeyset_t() : native_handle_(nullptr) {}
	hotkeyset_t(const hotkey* hotkeys, size_t num_hotkeys) { initialize(hotkeys, num_hotkeys); }
	~hotkeyset_t() { deinitialize(); }

	void initialize(const hotkey* hotkeys, size_t num_hotkeys);
	void deinitialize();

	template<size_t size> void initialize(const hotkey (&hotkeys)[size]) { initialize(hotkeys, size); }

protected:
	void* native_handle_;
};

// _____________________________________________________________________________
// Mouse (assumes Microsoft-style design)

class mouse_t
{
public:
	mouse_t();
	~mouse_t();

	void initialize();
	void deinitialize();
	
	// call this once per frame to analyze events received since last update call
	void update();

	// call this on lost focus or other major event to clear state
	void reset();

	// call this from an event generator and then update once all events have been
	// triggered
	void trigger(const mouse_event& e);
	
	inline peripheral_status status() const { return status_; }

	inline void enable(bool en) { status_ = en ? (status_ == peripheral_status::disabled ? peripheral_status::not_ready : status_) : peripheral_status::disabled; }
	inline bool enabled() const { return status_ != peripheral_status::disabled; }

	inline uint8_t down() const { return cur_dn; }
	inline uint8_t pressed() const { return cur_pr; }
	inline uint8_t released() const { return cur_re; }

	inline bool down(const mouse_button& b) const { return (down() & (uint8_t)b) != 0; }
	inline bool pressed(const mouse_button& b) const { return (pressed() & (uint8_t)b) != 0; }
	inline bool released(const mouse_button& b) const { return (released() & (uint8_t)b) != 0; }

	inline int16_t x() const { return cur_x; }
	inline int16_t y() const { return cur_y; }
	inline int16_t x_delta() const { return dlt_x; }
	inline int16_t y_delta() const { return dlt_y; }
	inline int16_t wheel_delta() const { return dlt_w; }

private:
	int16_t evt_x, evt_y, evt_dlt_x, evt_dlt_y, evt_dlt_w;
	int16_t cur_x, cur_y, dlt_x, dlt_y, dlt_w;
	int16_t cap_x, cap_y;
	uint8_t msg_dn, msg_up;
	uint8_t cur_dn, cur_pr, cur_re;
	peripheral_status status_;
};

// _____________________________________________________________________________
// Controller Pad (assumes PS2 Dual-Shock design)

class pad_t
{
public:
	static const uint32_t max_num_pads = 4;

	pad_t(uint8_t index = (uint8_t)-1);
	~pad_t();

	void initialize(uint8_t index);
	void deinitialize();

	void update();

	void reset();

	inline uint32_t index() const { return index_; }
	inline peripheral_status status() const { return status_; }

	inline void enable(bool en) { status_ = en ? (status_ == peripheral_status::disabled ? peripheral_status::not_ready : status_) : peripheral_status::disabled; }
	inline bool enabled() const { return status_ != peripheral_status::disabled; }

	inline uint16_t changed() const { return prev_down_ ^ down_; }
	inline uint16_t down() const { return down_; }
	inline uint16_t pressed() const { return pressed_; }
	inline uint16_t released() const { return released_; }

	inline bool changed(const pad_button& b) const { return (changed() & (uint16_t)b) != 0; }
	inline bool down(const pad_button& b) const { return (down() & (uint16_t)b) != 0; }
	inline bool pressed(const pad_button& b) const { return (pressed() & (uint16_t)b) != 0; }
	inline bool released(const pad_button& b) const { return (released() & (uint16_t)b) != 0; }

	inline float ljoy_x() const { return ljoy_x_; }
	inline float ljoy_y() const { return ljoy_y_; }
	inline float rjoy_x() const { return rjoy_x_; }
	inline float rjoy_y() const { return rjoy_y_; }
	inline float ltrigger() const { return ltrigger_; }
	inline float rtrigger() const { return rtrigger_; }

	inline float ljoy_x_delta() const { return ljoy_x_delta_; }
	inline float ljoy_y_delta() const { return ljoy_y_delta_; }
	inline float rjoy_x_delta() const { return rjoy_x_delta_; }
	inline float rjoy_y_delta() const { return rjoy_y_delta_; }
	inline float ltrigger_delta() const { return ltrigger_delta_; }
	inline float rtrigger_delta() const { return rtrigger_delta_; }

	// pad info doesn't come from the win32 message pump, so generate compatible 
	// events to pass it through any event abstraction
	bool trigger_events(void (*trigger)(const pad_event& e, void* user), void* user);

private:
	uint16_t prev_down_;
	uint16_t down_;
	uint16_t pressed_;
	uint16_t released_;
	float ljoy_x_;
	float ljoy_y_;
	float rjoy_x_;
	float rjoy_y_;
	float ljoy_x_delta_;
	float ljoy_y_delta_;
	float rjoy_x_delta_;
	float rjoy_y_delta_;
	float ltrigger_;
	float rtrigger_;
	float ltrigger_delta_;
	float rtrigger_delta_;
	uint8_t index_;
	peripheral_status prev_status_;
	peripheral_status status_;
};

// _____________________________________________________________________________
// Platform-specific support

#if defined(_WIN32)

// 0 if not handled, 1 if handled as a regular key event, -1 if a key even while alt is down (system key event). Alt always behaves as a normal key.
int handle_keyboard(void* hwnd, uint32_t msg, uintptr_t wparam, uintptr_t lparam, key_event* out = nullptr);

bool handle_hotkey(void* hwnd, uint32_t msg, uintptr_t wparam, uintptr_t lparam, struct hotkey_event* out);

// inout_context should be a value initialized to zero, then passed for read/write when this is called.
bool handle_mouse(void* hwnd, uint32_t msg, uintptr_t wparam, uintptr_t lparam, const mouse_capture& capture, uintptr_t* inout_context, mouse_event* out);

#endif

}
