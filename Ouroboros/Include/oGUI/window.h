// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// An abstraction for an operating system's window concept. This is fit for 
// containing child controls and doing other typical event-based Windowing. It 
// can also be made a primary render target for a GPU device. See oGPU for more
// details.

#pragma once
#include <oGUI/oGUI.h>
#include <oGUI/cursor.h>
#include <oConcurrency/future.h>
#include <oSystem/display.h>
#include <oSurface/image.h>

namespace ouro {

class keyboard_t;
class mouse_t;

class basic_window
{
public:
	// environmental API
	virtual window_handle native_handle() const = 0;
	virtual display::id display_id() const = 0;
	virtual bool is_window_thread() const = 0;
	
	// Flushes the window's message queue. This should be called in a loop on the
	// same thread where the window was created. If wait_for_next is true, this 
	// will block until at least one message wakes the thread up.
	virtual void flush_messages(bool wait_for_next = false) = 0;

	// Breaks out of a blocking flush_messages routine.
	virtual void quit() = 0;

	// If true, platform oTRACEs of every event and action will be enabled. This
	// is false by default.
	virtual void debug(bool debug) = 0;
	virtual bool debug() const = 0;


	// shape API
	virtual void state(window_state st) = 0;
	virtual window_state state() const = 0;
	inline void show(window_state st = window_state::restored) { state(st); }
	inline void hide() { state(window_state::hidden); }
	inline void minimize() { state(window_state::minimized); }
	inline void maximize() { state(window_state::maximized); }
	inline void restore() { state(window_state::restored); }

	virtual void client_position(const int2& pos) = 0;
	virtual int2 client_position() const = 0;
	virtual int2 client_size() const = 0;


	// border/decoration API
	virtual void icon(icon_handle icon) = 0;
	virtual icon_handle icon() const = 0;

	// sets the cursor to use when cursor_state::user is specified in client_cursor
	virtual void user_cursor(cursor_handle cursor) = 0;
	virtual cursor_handle user_cursor() const = 0;

	virtual void client_cursor(const mouse_cursor& c) = 0;
	virtual mouse_cursor client_cursor() const = 0;

	virtual void set_titlev(const char* format, va_list args) = 0;
	virtual char* get_title(char* dst, size_t dst_size) const = 0;

	inline void set_title(const char* format, ...) { va_list args; va_start(args, format); set_titlev(format, args); va_end(args); }
	template<size_t size> char* get_title(char (&dst)[size]) const { return get_title(dst, size); }
	template<size_t capacity> char* get_title(fixed_string<char, capacity>& dst) const { return get_title(dst, dst.capacity()); }


	// draw order/dependency API

	// Overrides shape and style to be RESTORED and NONE. If this exclusive 
	// fullscreen is true, then this throws an exception. A parent is 
	// automatically an owner of this window. If there is already an owner, this 
	// will throw an exception.
	virtual void parent(const std::shared_ptr<basic_window>& parent) = 0;
	virtual std::shared_ptr<basic_window> parent() const = 0;

	// An owner is more like a sibling who does all the work. Use this for the 
	// association between dialog boxes and the windows on top of which they are
	// applied. If there is a parent, this will throw an exception.
	virtual void owner(const std::shared_ptr<basic_window>& owner) = 0;
	virtual std::shared_ptr<basic_window> owner() const = 0;

	virtual void sort_order(window_sort_order order) = 0;
	virtual window_sort_order sort_order() const = 0;

	virtual void focus(bool f = true) = 0;
	virtual bool has_focus() const = 0;
};

class window : public basic_window
{
public:

	// events
	struct create_event;
	struct shape_event;
	struct timer_event;
	struct drop_event;
	struct input_device_event;
	struct custom_event;

	struct basic_event
	{
		basic_event(window_handle _hWindow, event_type type)
			: window(_hWindow)
			, type(type)
		{}

		// Native window handle
		window_handle window;

		// Event type. Use this to choose a downcaster below.
		event_type type;

		// union doesn't work because of int2's copy ctor so use downcasting instead
		inline const create_event& as_create() const;
		inline const shape_event& as_shape() const;
		inline const timer_event& as_timer() const;
		inline const drop_event& as_drop() const;
		inline const input_device_event& as_input_device() const;
		inline const custom_event& as_custom() const;
	};

	struct create_event : basic_event
	{
		create_event(window_handle _hWindow
			, statusbar_handle _hStatusBar
			, menu_handle _hMenu
			, const window_shape& _Shape
			, void* _pUser)
			: basic_event(_hWindow, event_type::creating)
			, statusbar(_hStatusBar)
			, menu(_hMenu)
			, shape(_Shape)
			, user(_pUser)
		{}

		// Native handle of the window's status bar.
		statusbar_handle statusbar;

		// Native handle of the top-level window's menu.
		menu_handle menu;
		window_shape shape;

		// The user can pass a value to this for usage during window creation.
		void* user;
	};

	struct shape_event : basic_event
	{
		shape_event(window_handle _hWindow
			, event_type type
			, const window_shape& _Shape)
			: basic_event(_hWindow, type)
			, shape(_Shape)
		{}

		window_shape shape;
	};

	struct timer_event : basic_event
	{
		timer_event(window_handle _hWindow, uintptr_t context)
			: basic_event(_hWindow, event_type::timer)
			, context(context)
		{}

		// Any pointer-sized value. It is recommended this be the address of a field
		// in the App's class that is the struct context for the timer event.
		uintptr_t context;
	};

	struct drop_event : basic_event
	{
		drop_event(window_handle _hWindow
			, const path_string* _pPaths
			, int _NumPaths
			, const int2& _ClientDropPosition)
			: basic_event(_hWindow, event_type::drop_files)
			, paths(_pPaths)
			, num_paths(_NumPaths)
			, client_drop_position(_ClientDropPosition)
		{}
		const path_string* paths;
		int num_paths;
		int2 client_drop_position;
	};

	struct input_device_event : basic_event
	{
		input_device_event(window_handle hwnd, input_type t, peripheral_status s, const char* name)
			: basic_event(hwnd, event_type::input_device_changed)
			, type(t)
			, status(s)
			, instance_name(name)
		{}

		input_type type;
		peripheral_status status;
		const char* instance_name;
	};

	struct custom_event : basic_event
	{
		custom_event(window_handle _hWindow, int _EventCode, uintptr_t context)
			: basic_event(_hWindow, event_type::custom_event)
			, code(_EventCode)
			, context(context)
		{}

		int code;
		uintptr_t context;
	};

	typedef std::function<void(const basic_event& _Event)> event_hook;

	
	// lifetime API
	struct init_t
	{
		init_t()
		: title("")
		, icon(nullptr)
		, keyboard(nullptr)
		, mouse(nullptr)
		, create_user_data(nullptr)
		, user_cursor(nullptr)
		, cursor(mouse_cursor::arrow)
		, sort_order(window_sort_order::sorted)
		, debug(false)
		, allow_controller(false)
		, allow_touch(false)
		, client_drag_to_move(false)
		, alt_f4_closes(false)
	{}
  
		const char* title;
		icon_handle icon;
		keyboard_t* keyboard; // keyboard events will trigger on this object
		mouse_t* mouse; // mouse events will trigger on this object
		void* create_user_data; // user data accessible in the create event
		cursor_handle user_cursor; // if mouse_cursor::user is specified
		mouse_cursor cursor;
		window_sort_order sort_order;
		bool debug;
		bool allow_controller;
		bool allow_touch;
		bool client_drag_to_move;
		bool alt_f4_closes;

		// NOTE: The event_type::creating event gets fired during construction, so if that 
		// event is to be hooked it needs to be passed and hooked up during 
		// construction.
		event_hook on_event;
		input_hook on_input;
		window_shape shape;
	};

	static std::shared_ptr<window> make(const init_t& i);

	// returns the init_t used to create the window
	virtual const init_t& init() const = 0;


	// shape API
	virtual void shape(const window_shape& _Shape) = 0;
	virtual window_shape shape() const = 0;
	void state(window_state st) override { window_shape s; s.state = st; shape(s); }
	window_state state() const override { window_shape s = shape(); return s.state; }
	void style(window_style _Style) { window_shape s; s.style = _Style; shape(s); }
	window_style style() const { window_shape s = shape(); return s.style; }

	void client_position(const int2& pos) override { window_shape s; s.client_position = pos; shape(s); }
	int2 client_position() const override { window_shape s = shape(); return s.client_position; }
	void client_size(const int2& _ClientSize) { window_shape s; s.client_size = _ClientSize; shape(s); }
	int2 client_size() const override { window_shape s = shape(); return s.client_size; }
 
  
	// border/decoration API

	// For widths, if the last width is -1, it will be interpreted as "take up 
	// rest of width of window".
	virtual void set_num_status_sections(const int* _pStatusSectionWidths, size_t _NumStatusSections) = 0;
	virtual int get_num_status_sections(int* _pStatusSectionWidths = nullptr, size_t _MaxNumStatusSectionWidths = 0) const = 0;
	template<size_t size> void set_num_status_sections(const int (&_pStatusSectionWidths)[size]) { set_num_status_sections(_pStatusSectionWidths, size); }
	template<size_t size> int get_num_status_sections(int (&_pStatusSectionWidths)[size]) { return get_num_status_sections(_pStatusSectionWidths, size); }

	virtual void set_status_textv(int _StatusSectionIndex, const char* format, va_list args) = 0;
	virtual char* get_status_text(char* dst, size_t dst_size, int _StatusSectionIndex) const = 0;

	inline void set_status_text(int _StatusSectionIndex, const char* format, ...) { va_list args; va_start(args, format); set_status_textv(_StatusSectionIndex, format, args); va_end(args); }
	template<size_t size> char* get_status_text(char (&dst)[size], int _StatusSectionIndex) const { return get_status_text(dst, size, _StatusSectionIndex); }
	template<size_t capacity> char* get_status_text(fixed_string<char, capacity>& dst, int _StatusSectionIndex) const { return get_status_text(dst, dst.capacity(), _StatusSectionIndex); }

	virtual void status_icon(int _StatusSectionIndex, icon_handle icon) = 0;
	virtual icon_handle status_icon(int _StatusSectionIndex) const = 0;

 
	// extended input API

	// Returns true if this window has been associated with a rendering API that
	// may take control and reformat the window. This is manually flagged/specified
	// to keep GUI and rendering code orthogonal.
	virtual void render_target(bool _RenderTarget) = 0;
	virtual bool render_target() const = 0;

	// If true, platform oTRACEs of every event and input will be enabled. This
	// is false by default.
	virtual void debug(bool debug) = 0;
	virtual bool debug() const = 0;

	// If true, a timer will be set up internally that will update the state of 
	// any controllers attached and send inputs as appropriate.
	virtual void allow_controller_input(bool allow = true) = 0;
	virtual bool allow_controller_input() const = 0;
	
	// Many touch screen drivers emulated mouse events in a way that is not 
	// consistent with regular mouse behavior, so if that affects application 
	// logic, here's a switch. This is false by default.
	virtual void allow_touch_input(bool allow) = 0;
	virtual bool allow_touch_input() const = 0;

	// If set to true, any mouse input in the client area will move the window.
	// This is useful for borderless windows like splash screens. This is false by 
	// default.
	virtual void client_drag_to_move(bool drag_moves) = 0;
	virtual bool client_drag_to_move() const = 0;

	// If set to true, Alt-F4 will trigger an event_type::closing event. This is true by 
	// default.
	virtual void alt_f4_closes(bool alt_f4_closes) = 0;
	virtual bool alt_f4_closes() const = 0;

	virtual void enabled(bool enabled) = 0;
	virtual bool enabled() const = 0;

	virtual void capture(const mouse_capture& type) = 0;
	virtual mouse_capture capture() const = 0;

	// Defines a set of hotkeys. Use set_hotkeys to activate one set at a time.
	virtual int new_hotkeys(const hotkey* hotkeys, size_t num_hotkeys) = 0;
	template<size_t size> int new_hotkeys(const hotkey (&hotkeys)[size]) { return new_hotkeys(hotkeys, size); }

	virtual void del_hotkeys(int hotkeys_id) = 0;
	virtual void set_hotkeys(int hotkeys_id) = 0;

	virtual int get_hotkeys(int hotkey_id, hotkey* hotkeys, size_t max_num_hotkeys) const = 0;
	template<size_t size> int get_hotkeys(int hotkey_id, hotkey (&hotkeys)[size]) { return get_hotkeys(hotkey_id, hotkeys, size); }


	// observer API

	virtual int hook_input(const input_hook& hook) = 0;
	virtual void unhook_input(int input_hook_id) = 0;

	virtual int hook_events(const event_hook& hook) = 0;
	virtual void unhook_events(int event_hook_id) = 0;


	// execution API

	// Appends a broadcast of an action as if it came from user input.
	virtual void trigger(const input_t& input) = 0;

	// Post an event that is specified by the user here.
	virtual void post(int custom_event_code, uintptr_t context) = 0;

	// Posts the specified task in the window's message queue and executes it in 
	// order with other events. This is useful for wrapping platform-specific 
	// window/control calls.
	virtual void dispatch(const std::function<void()>& task) = 0;
	oDEFINE_CALLABLE_WRAPPERS(,dispatch,, dispatch);

	// Schedules an oImage to be generated from the window. In the simple case,
	// frame is not used and the front buffer is captured. Due to platform rules
	// this may involve bringing the specified window to focus.
	virtual future<surface::image> snapshot(int frame = -1, bool include_border = false) const = 0;

	// Causes an event_type::timer event to occur with the specified context after the 
	// specified time. This will be called every specified milliseconds until 
	// stop_timer is called with the same context.
	virtual void start_timer(uintptr_t context, unsigned int relative_time_ms) = 0;
	virtual void stop_timer(uintptr_t context) = 0;
};

const window::create_event& window::basic_event::as_create() const { oAssert(type == event_type::creating, "wrong type"); return *static_cast<const create_event*>(this); }
const window::shape_event& window::basic_event::as_shape() const { oAssert(is_shape_event(type), "wrong type"); return *static_cast<const shape_event*>(this); }
const window::timer_event& window::basic_event::as_timer() const { oAssert(type == event_type::timer, "wrong type"); return *static_cast<const timer_event*>(this); }
const window::drop_event& window::basic_event::as_drop() const { oAssert(type == event_type::drop_files, "wrong type"); return *static_cast<const drop_event*>(this); }
const window::input_device_event& window::basic_event::as_input_device() const { oAssert(type == event_type::input_device_changed, "wrong type"); return *static_cast<const input_device_event*>(this); }
const window::custom_event& window::basic_event::as_custom() const { oAssert(type == event_type::custom_event, "wrong type"); return *static_cast<const custom_event*>(this); }

}
