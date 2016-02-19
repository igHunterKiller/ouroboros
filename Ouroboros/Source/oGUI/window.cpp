// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oCore/algorithm.h>
#include <oGUI/window.h>
#include <oGUI/menu.h>
#include <oGUI/Windows/win_as_string.h>
#include <oGUI/Windows/oWinRect.h>
#include <oGUI/Windows/oWinStatusBar.h>
#include <oGUI/Windows/oWinWindowing.h>
#include <oSystem/peripherals.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/windows/win_util.h>
#include <oSurface/codec.h>
#include <oConcurrency/backoff.h>
#include <oConcurrency/event.h>
#include <vector>
#include <commctrl.h>
#include <windowsx.h>
#include <Shellapi.h>

namespace ouro {

static bool kForceDebug = false;

#define DISPATCH(_SimpleFunction) do { dispatch_internal(std::move([=] { _SimpleFunction; })); } while(false)

static bool handle_menu(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, menu_t* out)
{
	if (msg == WM_COMMAND && !lparam && HIWORD(wparam) == 0)
	{
		out->window = hwnd;
		out->id = LOWORD(wparam);
		return true;
	}
	return false;
}

static bool handle_control(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, control_t* out)
{
	switch (msg)
	{
		case WM_COMMAND:
			if (lparam)
			{
				out->window = (window_handle)lparam;
				out->native_status = HIWORD(wparam);
				out->id = LOWORD(wparam);
				out->status = control_status::activated;
				return true;
			}
			break;

		case WM_HSCROLL:
			if (lparam)
			{
				out->window = (window_handle)lparam;
				out->native_status = uint32_t(-1);
				out->id = int16_t(-1);
				out->status = LOWORD(wparam) == TB_ENDTRACK ? control_status::deactivated : control_status::activated;
			}

			return false; // allow fall-thru to default message handler
		
		case WM_NOTIFY:
		{
			const NMHDR& nmhdr = *(const NMHDR*)lparam;
			oAssert((nmhdr.idFrom & 0xffff) == nmhdr.idFrom, "losing info");
			control_status status = control_status::count;

			control_type::value type = oWinControlGetType(nmhdr.hwndFrom);
			switch (type)
			{
				case control_type::tab:
				{
					switch (nmhdr.code)
					{
						case TCN_SELCHANGING: status = control_status::selection_changing; break;
						case TCN_SELCHANGE: status = control_status::selection_changed; break;
						default: break;
					}

					break;
				}

				case control_type::button:
				{
					switch (nmhdr.code)
					{
						case BN_CLICKED: status = control_status::activated; break;
						default: break;
					}

					break;
				}

				case control_type::hyperlabel:
				{
					switch (nmhdr.code)
					{
						case NM_CLICK: status = control_status::activated; break;
						default: break;
					}

					break;
				}

				default: break;
			}

			if (status == control_status::count)
				return false;

			out->window = nmhdr.hwndFrom;
			out->native_status = nmhdr.code;
			out->id = int16_t(nmhdr.idFrom);
			out->status = status;

			LRESULT lresult = FALSE;
			return !oWinControlDefaultOnNotify(hwnd, nmhdr, &lresult, type);
		}

		default: break;
	}

	return false;
}

static bool handle_touch(UINT msg, WPARAM wparam, LPARAM lparam, touch_t* out)
{
#ifdef oWINDOWS_HAS_REGISTERTOUCHWINDOW
	if (msg == WM_TOUCH)
	{
		TOUCHINPUT inputs[10];
		const UINT ntouches = __min(LOWORD(wparam), countof(inputs));
		if (ntouches)
		{
			if (GetTouchInputInfo((HTOUCHINPUT)lparam, ntouches, inputs, sizeof(TOUCHINPUT)))
			{
				for (int i = 0; i < countof(inputs); i++)
				{
					out->x[i] = (int16_t)inputs[i].x;
					out->y[i] = (int16_t)inputs[i].y;
				}

				CloseTouchInputHandle((HTOUCHINPUT)lparam);
			}
		}
		return true;
	}
#endif
	return false;
}

static bool handle_skeleton(UINT msg, WPARAM wparam, LPARAM lparam, skeleton_t* out)
{
	skeleton_status status = skeleton_status::count;

	switch (msg)
	{
		case oWM_SKELETON: status = skeleton_status::update; break;
		case oWM_USER_CAPTURED: status = skeleton_status::acquired; break;
		case oWM_USER_LOST: status = skeleton_status::lost; break;
		default: break;
	}

	if (status != skeleton_status::count)
	{
		out->status = status;
		out->index = (uint8_t)LOWORD(wparam);
		out->handle = (void*)lparam;
		return true;
	}

	return false;
}

template<typename handleT>
class handle_manager
{
public:
	typedef handleT handle_type;
	typedef std::recursive_mutex mutex_t;
	typedef std::lock_guard<mutex_t> lock_t;

	handle_manager() { handles.reserve(8); }

	int add(const handle_type& handle)
	{
		lock_t lock(mtx);
		return static_cast<int>(sparse_set(handles, handle));
	}

	void remove(int handle_id)
	{
		lock_t lock(mtx);
		ranged_set(handles, handle_id, nullptr);
	}

	size_t size() const { return handles.size(); }

	const handleT& operator[](int i) const { return handles[i]; }
	handleT&       operator[](int i)       { return handles[i]; }

private:
	mutex_t mtx;
	std::vector<handle_type> handles;
};

template<typename HookT, typename ParamT>
class hook_manager
{
public:
	typedef HookT hook_type;
	typedef ParamT param_type;
	typedef std::recursive_mutex mutex_t;
	typedef std::lock_guard<mutex_t> lock_t;

	hook_manager() { hooks.reserve(8); }

	int hook(const hook_type& hook)
	{
		lock_t lock(mtx);
		return static_cast<int>(sparse_set(hooks, hook));
	}

	void unhook(int hook_id)
	{
		lock_t lock(mtx);
		ranged_set(hooks, hook_id, nullptr);
	}

	void visit(const param_type& param)
	{
		lock_t lock(mtx);
		for (hook_type& hook : hooks)
			if (hook)
				hook(param);
	}

private:
	mutex_t mtx;
	std::vector<hook_type> hooks;
};

struct window_impl : window
{
	oDECLARE_WNDPROC(window_impl);

	window_impl(const init_t& i);
	~window_impl();
	
	// environmental API
	window_handle native_handle() const override;
	display::id display_id() const override;
	bool is_window_thread() const override;
	void render_target(bool _RenderTarget) override;
	bool render_target() const override;
	void debug(bool debug = true) override;
	bool debug() const override;
	void flush_messages(bool wait_for_next = false) override;
	void quit() override;

	const init_t& init() const override;


	// shape API
	void shape(const window_shape& _Shape) override;
	window_shape shape() const override;

	// border/decoration API
	void icon(icon_handle _hIcon) override;
	icon_handle icon() const override;
	void user_cursor(cursor_handle _hCursor) override;
	cursor_handle user_cursor() const override;
	void client_cursor(const mouse_cursor& c) override;
	mouse_cursor client_cursor() const override;
	void set_titlev(const char* format, va_list args) override;
	char* get_title(char* _StrDestination, size_t _SizeofStrDestination) const override;
	void set_num_status_sections(const int* _pStatusSectionWidths, size_t _NumStatusSections) override;
	int get_num_status_sections(int* _pStatusSectionWidths = nullptr, size_t _MaxNumStatusSectionWidths = 0) const override;
	void set_status_textv(int _StatusSectionIndex, const char* format, va_list args) override;
	char* get_status_text(char* _StrDestination, size_t _SizeofStrDestination, int _StatusSectionIndex) const override;
	void status_icon(int _StatusSectionIndex, icon_handle _hIcon) override;
	icon_handle status_icon(int _StatusSectionIndex) const override;

	// Draw Order/Dependency API
	void parent(const std::shared_ptr<basic_window>& _Parent) override;
	std::shared_ptr<basic_window> parent() const override;
	void owner(const std::shared_ptr<basic_window>& owner) override;
	std::shared_ptr<basic_window> owner() const override;
	void sort_order(window_sort_order::value order) override;
	window_sort_order::value sort_order() const override;
	void focus(bool f = true) override;
	bool has_focus() const override;


	// Extended Input API
	void allow_controller_input(bool allow = true) override;
	bool allow_controller_input() const override;
	void allow_touch_input(bool allow = true) override;
	bool allow_touch_input() const override;
	void client_drag_to_move(bool drag_moves = true) override;
	bool client_drag_to_move() const override;
	void alt_f4_closes(bool alt_f4_closes = true) override;
	bool alt_f4_closes() const override;
	void enabled(bool enabled) override;
	bool enabled() const override;
	void capture(const mouse_capture& type) override;
	mouse_capture capture() const override;
	int new_hotkeys(const ouro::hotkey* hotkeys, size_t num_hotkeys) override;
	void del_hotkeys(int hotkeys_id) override;
	void set_hotkeys(int hotkeys_id) override;
	int get_hotkeys(int hotkey_id, ouro::hotkey* hotkeys, size_t max_num_hotkeys) const override;

	// Observer API
	int hook_input(const input_hook& hook) override;
	void unhook_input(int input_hook_id) override;
	int hook_events(const event_hook& hook) override;
	void unhook_events(int event_hook_id) override;

	// Execution API
	void trigger(const input_t& input) override;
	void post(int custom_event_code, uintptr_t context) override;
	void dispatch(const std::function<void()>& task) override;
	future<surface::image> snapshot(int frame = -1, bool include_border = false) const override;
	void start_timer(uintptr_t context, unsigned int relative_time_ms) override;
	void stop_timer(uintptr_t context) override;

private:
	
	HWND hWnd;
	HANDLE hHeap;
	cursor_handle hUserCursor;

	std::shared_ptr<basic_window> Owner;
	std::shared_ptr<basic_window> Parent;

	mouse_cursor ClientCursorState;
	mouse_capture CaptureType;
	int16_t MouseAtCaptureX;
	int16_t MouseAtCaptureY;
	uintptr_t MouseContext;

	pad_t pads[4];
	keyboard_t* keyboard;
	mouse_t* mouse;

	window_sort_order::value SortOrder;
	bool ClientDragToMove;
	bool Debug;
	bool AllowControllers;
	bool AllowTouch;
	bool SkipNextMouseMove;

	window_shape PriorShape;

	typedef handle_manager<keyboard_t::hotkeyset_t> hotkeyset_manager_t;
	typedef hook_manager<input_hook, input_t> input_manager_t;
	typedef hook_manager<event_hook, basic_event> event_manager_t;

	hotkeyset_manager_t HotkeySets;
	int CurrentHotkeySet;
	input_manager_t InputHooks;
	event_manager_t EventHooks;

	event Destroyed;

	init_t Init;

private:

	void dispatch_internal(std::function<void()>&& task) const { PostMessage(hWnd, oWM_DISPATCH, 0, (LPARAM)const_cast<window_impl*>(this)->new_object<std::function<void()>>(std::move(task))); }

	struct scoped_heap_lock
	{
		scoped_heap_lock(HANDLE heap) : h(heap) { HeapLock(heap); }
		~scoped_heap_lock() { HeapUnlock(h); }
		private: HANDLE h;
	};

	template<typename T>
	T* new_object(const T& obj) { scoped_heap_lock lock(hHeap); return new (HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, sizeof(T))) T(obj); }
	
	template<typename T>
	T* new_object(T&& obj) { scoped_heap_lock lock(hHeap); return new (HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, sizeof(T))) T(std::move(obj)); }

	template<typename T>
	void delete_object(T* obj) { obj->~T(); scoped_heap_lock lock(hHeap); HeapFree(hHeap, HEAP_GENERATE_EXCEPTIONS, obj); }

	template<typename T>
	T* new_array(size_t num_objs)
	{
		void* p = nullptr;
		{
			scoped_heap_lock lock(hHeap);
			p = HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, sizeof(T) * num_objs + sizeof(size_t));
		}
		*(size_t*)p = num_objs;

		T* pObjects = (T*)((uint8_t*)p + sizeof(size_t));
		for (size_t i = 0; i < num_objs; i++)
			pObjects[i] = std::move(T());
		return pObjects;
	}

	template<typename T>
	void delete_array(T* objs)
	{
		size_t* pNumObjects = (size_t*)((uint8_t*)objs - sizeof(size_t));
		*pNumObjects;
		for (size_t i = 0; i < *pNumObjects; i++)
			objs[i].~T();
		scoped_heap_lock lock(hHeap);
		HeapFree(hHeap, HEAP_GENERATE_EXCEPTIONS, pNumObjects);
	}

	char* new_string(const char* format, va_list args)
	{
		const static size_t kStartLength = 1024;
		const static size_t kStopLength = 128*1024;
		char* s = nullptr;
		int len = -1;
		size_t cap = kStartLength;
		while (len < 0 && cap < kStopLength)
		{
			scoped_heap_lock lock(hHeap);
			s = (char*)HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS, cap);
			len = vsnprintf(s, cap, format, args);
			if (len >= 0 && size_t(len) < cap)
				break;
			HeapFree(hHeap, HEAP_GENERATE_EXCEPTIONS, s);
			cap *= 2;
		}

		oCheck(len >= 0, std::errc::invalid_argument, "formatting failed for string \"%s\"", format);
		return s;
	}

	void delete_string(char* str)
	{
		scoped_heap_lock lock(hHeap);
		HeapFree(hHeap, HEAP_GENERATE_EXCEPTIONS, str);
	}

	void init_window(const init_t& i);
	void trigger_generic_event(event_type::value _Event, window_shape* _pShape = nullptr);
	void set_cursor();
	bool handle_input(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult);
	bool handle_sizing(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult);
	bool handle_misc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult);
	bool handle_lifetime(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult);
	
	void handle_controllers();
};

void window_impl::init_window(const init_t& i)
{
	// this->hWnd assigned in WM_CREATE
	oWinCreate(nullptr, i.title, i.shape.style, i.shape.client_position, i.shape.client_size, StaticWndProc, (void*)&i, this);

	if (i.debug)
	{
		oTrace("HWND 0x%x '%s' running on thread %d (0x%x)"
			, hWnd
			, oSAFESTRN(i.title)
			, asdword(std::this_thread::get_id())
			, asdword(std::this_thread::get_id()));
	}
	
	PriorShape = oWinGetShape(hWnd);

	// Initialize decoration

	if (i.icon)
		oWinSetIcon(hWnd, (HICON)i.icon);

	// Still have to set style here since oWinCreate is still unaware of menu/status bar
	window_shape InitShape;
	InitShape.state = i.shape.state;
	InitShape.style = i.shape.style;

	oWinSetShape(hWnd, InitShape);
}

window_impl::window_impl(const init_t& i)
	: hWnd(nullptr)
	, hHeap(HeapCreate(HEAP_GENERATE_EXCEPTIONS, 1 * 1024 * 1024, 0))
	, hUserCursor(nullptr)
	, CaptureType(mouse_capture::none)
	, keyboard(i.keyboard)
	, mouse(i.mouse)
	, ClientCursorState(i.cursor)
	, SortOrder(i.sort_order)
	, MouseContext(0)
	, ClientDragToMove(false)
	, Debug(false)
	, AllowControllers(false)
	, AllowTouch(false)
	, SkipNextMouseMove(false)
	, MouseAtCaptureX(0)
	, MouseAtCaptureY(0)
	, CurrentHotkeySet(-1)
	, Init(i)
{
	if (i.on_input)
		InputHooks.hook(i.on_input);

	if (i.on_event)
		EventHooks.hook(i.on_event);

	init_window(i);
	window_impl::sort_order(SortOrder);
	window_impl::debug(i.debug);
	window_impl::allow_controller_input(i.allow_controller);
	window_impl::allow_touch_input(i.allow_touch);
	window_impl::client_drag_to_move(i.client_drag_to_move);
	window_impl::alt_f4_closes(i.alt_f4_closes);
}

window_impl::~window_impl()
{
	if (hWnd)
	{
		// If not, probably because a parent window destroyed it
		if (oWinExists(hWnd))
		{
			oWinDestroy(hWnd);
			Destroyed.wait();
		}

		else
			hWnd = nullptr;
	}

	if (hHeap)
		HeapDestroy(hHeap);
}

std::shared_ptr<window> window::make(const init_t& i)
{
	return std::make_shared<window_impl>(i);
}

const window::init_t& window_impl::init() const
{
	return Init;
}

window_handle window_impl::native_handle() const
{
	return (window_handle)hWnd;
}

display::id window_impl::display_id() const
{
	window_shape s = shape();
	int2 center = s.client_position + s.client_size / 2;
	return display::find(center.x, center.y);
}

bool window_impl::is_window_thread() const
{
	return oWinIsWindowThread(hWnd);
}

void window_impl::shape(const window_shape& _Shape)
{
	dispatch_internal(std::move([=]
	{
		try { oWinSetShape(hWnd, _Shape); }
		catch (std::exception& e)
		{ e; oTraceA("ERROR: oWinSetShape: %s", e.what()); }
	}));
}

window_shape window_impl::shape() const
{
	return oWinGetShape(hWnd);
}

void window_impl::icon(icon_handle _hIcon)
{
	DISPATCH(oWinSetIcon(hWnd, (HICON)_hIcon, true));
	DISPATCH(oWinSetIcon(hWnd, (HICON)_hIcon, false));
}

icon_handle window_impl::icon() const
{
	return (icon_handle)oWinGetIcon(hWnd);
}

void window_impl::user_cursor(cursor_handle _hCursor)
{
	dispatch_internal(std::move([=]
	{
		if (hUserCursor)
			DestroyCursor((HCURSOR)hUserCursor);
		hUserCursor = _hCursor;
	}));
}

cursor_handle window_impl::user_cursor() const
{
	return (cursor_handle)hUserCursor;
}

void window_impl::client_cursor(const mouse_cursor& c)
{
	dispatch_internal(std::move([=]
	{
		ClientCursorState = c;
		set_cursor();
	}));
}

mouse_cursor window_impl::client_cursor() const
{
	return ClientCursorState;
}

void window_impl::set_titlev(const char* format, va_list args)
{
	char* pString = new_string(format, args);
	dispatch_internal(std::move([=]
	{
		oWinSetText(hWnd, pString);
		if (pString)
			delete_string(pString);
	}));
}

char* window_impl::get_title(char* _StrDestination, size_t _SizeofStrDestination) const
{
	return oWinGetText(_StrDestination, _SizeofStrDestination, hWnd);
}

void window_impl::set_num_status_sections(const int* _pStatusSectionWidths, size_t _NumStatusSections)
{
	window_impl* w = const_cast<window_impl*>(this);
	int* pCopy = w->new_array<int>(_NumStatusSections);
	memcpy(pCopy, _pStatusSectionWidths, _NumStatusSections * sizeof(int));
	dispatch_internal(std::move([=]
	{
		oWinStatusBarSetNumItems(oWinGetStatusBar(hWnd), pCopy, _NumStatusSections);
		if (pCopy)
			delete_array(pCopy);
	}));
}

int window_impl::get_num_status_sections(int* _pStatusSectionWidths, size_t _MaxNumStatusSectionWidths) const
{
	return oWinStatusBarGetNumItems(oWinGetStatusBar(hWnd), _pStatusSectionWidths, _MaxNumStatusSectionWidths);
}

void window_impl::set_status_textv(int _StatusSectionIndex, const char* format, va_list args)
{
	char* pString = new_string(format, args);
	dispatch_internal(std::move([=]
	{
		oWinStatusBarSetText(oWinGetStatusBar(hWnd), _StatusSectionIndex, border_style::flat, pString);
		if (pString)
			delete_string(pString);
	}));
}

char* window_impl::get_status_text(char* _StrDestination, size_t _SizeofStrDestination, int _StatusSectionIndex) const
{
	size_t len = 0;
	window_impl* w = const_cast<window_impl*>(this);
	std::function<void()>* pTask = w->new_object<std::function<void()>>(std::move([=,&len]
	{
		if (oWinStatusBarGetText(_StrDestination, _SizeofStrDestination, oWinGetStatusBar(hWnd), _StatusSectionIndex))
			len = strlen(_StrDestination);
	}));

	oTrace("get_status_text");
	SendMessage(hWnd, oWM_DISPATCH, 0, (LPARAM)pTask);
	return (len && len <= _SizeofStrDestination) ? _StrDestination : nullptr;
}

void window_impl::status_icon(int _StatusSectionIndex, icon_handle _hIcon)
{
	DISPATCH(oWinStatusBarSetIcon(oWinGetStatusBar(hWnd), _StatusSectionIndex, (HICON)_hIcon));
}

icon_handle window_impl::status_icon(int _StatusSectionIndex) const
{
	return (icon_handle)oWinStatusBarGetIcon(oWinGetStatusBar(hWnd), _StatusSectionIndex);
}

void window_impl::parent(const std::shared_ptr<basic_window>& _Parent)
{
	dispatch_internal(std::move([=]
	{
		if (Owner)
			oThrow(std::errc::invalid_argument, "Can't have owner at same time as parent");
		Parent = _Parent;
		oWinSetParent(hWnd, Parent ? (HWND)Parent->native_handle() : nullptr);
	}));
}

std::shared_ptr<basic_window> window_impl::parent() const
{
	return const_cast<window_impl*>(this)->Parent;
}

void window_impl::owner(const std::shared_ptr<basic_window>& owner)
{
	dispatch_internal(std::move([=]
	{
		if (Parent)
			oThrow(std::errc::invalid_argument, "Can't have parent at same time as owner");

		Owner = owner;
		oWinSetOwner(hWnd, Owner ? (HWND)Owner->native_handle() : nullptr);
	}));
}

std::shared_ptr<basic_window> window_impl::owner() const
{
	return const_cast<window_impl*>(this)->Owner;
}

void window_impl::sort_order(window_sort_order::value order)
{
	dispatch_internal(std::move([=]
	{
		this->SortOrder = order;
		oWinSetAlwaysOnTop(hWnd, order != window_sort_order::sorted);
		if (order == window_sort_order::always_on_top_with_focus)
			::SetTimer(hWnd, (UINT_PTR)&SortOrder, 500, nullptr);
		else
			::KillTimer(hWnd, (UINT_PTR)&SortOrder);
	}));
}

window_sort_order::value window_impl::sort_order() const
{
	return SortOrder;
}

void window_impl::focus(bool f)
{
	DISPATCH(oWinSetFocus(hWnd));
}

bool window_impl::has_focus() const
{
	return oWinHasFocus(hWnd);
}

void window_impl::render_target(bool _RenderTarget)
{
	oWinSetIsRenderTarget(hWnd, _RenderTarget);
}

bool window_impl::render_target() const
{
	return oWinIsRenderTarget(hWnd);
}

void window_impl::debug(bool debug)
{
	dispatch_internal([=] { Debug = debug; });
}

bool window_impl::debug() const
{
	return Debug;
}

void window_impl::allow_controller_input(bool allow)
{
	dispatch_internal([=]
	{
		AllowControllers = allow;

		for (uint32_t i = 0; i < countof(pads); i++)
			pads[i].enable(allow);

		if (allow)
			::SetTimer(hWnd, (UINT_PTR)pads, 33, nullptr);
		else
			::KillTimer(hWnd, (UINT_PTR)pads);
	});
}

bool window_impl::allow_controller_input() const
{
	return pads[0].enabled();
}

void window_impl::allow_touch_input(bool allow)
{
	dispatch_internal([=]
	{
		oWinRegisterTouchEvents(hWnd, allow);
		AllowTouch = allow;
	});
}

bool window_impl::allow_touch_input() const
{
	return AllowTouch;
}

void window_impl::client_drag_to_move(bool drag_moves)
{
	dispatch_internal([=] { ClientDragToMove = drag_moves; });
}

bool window_impl::client_drag_to_move() const
{
	return ClientDragToMove;
}

void window_impl::alt_f4_closes(bool alt_f4_closes)
{
	DISPATCH(oWinAltF4Enable(hWnd, alt_f4_closes));
}

bool window_impl::alt_f4_closes() const
{
	return oWinAltF4IsEnabled(hWnd);
}

void window_impl::enabled(bool enabled)
{
	DISPATCH(oWinEnable(hWnd, enabled));
}

bool window_impl::enabled() const
{
	return oWinIsEnabled(hWnd);
}

void window_impl::capture(const mouse_capture& type)
{
	dispatch_internal([=]
	{
		CaptureType = type;
		cursor::capture(CaptureType == mouse_capture::none ? nullptr : hWnd, &MouseAtCaptureX, &MouseAtCaptureY);
	});
}

mouse_capture window_impl::capture() const
{
	return CaptureType;
}

int window_impl::new_hotkeys(const ouro::hotkey* hotkeys, size_t num_hotkeys)
{
	auto hotkeyset = keyboard_t::new_hotkeyset(hotkeys, num_hotkeys);
	return HotkeySets.add(hotkeyset);
}

void window_impl::del_hotkeys(int hotkeys_id)
{
	return HotkeySets.remove(hotkeys_id);
}

void window_impl::set_hotkeys(int hotkeys_id)
{
	dispatch_internal([=] { CurrentHotkeySet = hotkeys_id; });
}

int window_impl::get_hotkeys(int hotkey_id, ouro::hotkey* hotkeys, size_t max_num_hotkeys) const
{
	auto hotkeyset = HotkeySets[hotkey_id];
	return keyboard_t::decode_hotkeyset(hotkeyset, hotkeys, max_num_hotkeys);
}

int window_impl::hook_input(const input_hook& hook)
{
	return InputHooks.hook(hook);
}

void window_impl::unhook_input(int input_hook_id)
{
	InputHooks.unhook(input_hook_id);
}

int window_impl::hook_events(const event_hook& hook)
{
	return EventHooks.hook(hook);
}

void window_impl::unhook_events(int event_hook_id)
{
	EventHooks.unhook(event_hook_id);
}

void window_impl::trigger(const input_t& input)
{
	dispatch_internal(std::move(std::bind(&input_manager_t::visit, &InputHooks, input))); // bind by copy
}

void window_impl::post(int custom_event_code, uintptr_t context)
{
	custom_event e((window_handle)hWnd, custom_event_code, context);
	dispatch_internal(std::bind(&event_manager_t::visit, &EventHooks, e)); // bind by copy
}

void window_impl::dispatch(const std::function<void()>& task)
{
	std::function<void()>* pTask = new_object<std::function<void()>>(task);
	oTrace("dispatch");
	PostMessage(hWnd, oWM_DISPATCH, 0, (LPARAM)pTask);
}

static bool oWinWaitUntilOpaque(HWND hwnd, unsigned int _TimeoutMS)
{
	backoff bo;
	unsigned int Now = timer::now_msi();
	unsigned int Then = Now + _TimeoutMS;
	while (!oWinIsOpaque(hwnd))
	{
		bo.pause();
		Now = timer::now_msi();
		if (_TimeoutMS != ~0u && Now > Then)
			return false;
	}

	return true;
}

future<surface::image> window_impl::snapshot(int frame, bool include_border) const
{
	auto PromisedSnap = std::make_shared<ouro::promise<surface::image>>();
	auto Image = PromisedSnap->get_future();

	const_cast<window_impl*>(this)->dispatch([=]() mutable
	{
		bool success = oWinWaitUntilOpaque(hWnd, 20000);
		if (!success)
		{
			window_shape s = oWinGetShape(hWnd);
			if (is_visible(s.state))
				oThrow(std::errc::invalid_argument, ""); // pass through verification of wait
			else
				oThrow(std::errc::invalid_argument, "A non-hidden window timed out waiting to become opaque");
		}

		surface::image snap;
		void* buf = nullptr;
		size_t size = 0;
		oWinSetFocus(hWnd); // Windows doesn't do well with hidden contents.
		try
		{
			oGDIScreenCaptureWindow(hWnd, include_border, malloc, &buf, &size, false, false);
			snap = surface::decode(buf, size);
			free(buf);
		}

		catch (std::exception)
		{
			PromisedSnap->set_exception(std::current_exception());
			snap.deinitialize();
		}

		if (!!snap)
			PromisedSnap->set_value(std::move(snap));
	});

	return Image;
}

void window_impl::start_timer(uintptr_t context, unsigned int relative_time_ms)
{
	DISPATCH(::SetTimer(hWnd, (UINT_PTR)context, relative_time_ms, nullptr));
}

void window_impl::stop_timer(uintptr_t context)
{
	DISPATCH(::KillTimer(hWnd, (UINT_PTR)context));
}

void window_impl::handle_controllers()
{
	for (uint8_t i = 0; i < countof(pads); i++)
	{
		pad_t& c = pads[i];
		c.update();

		if (c.enabled())
		{
			c.trigger_events([](const pad_event& e, void* user)
			{
				input_t input;
				input.type = input_type::pad;
				input.status = peripheral_status::full_power; // todo: translate status
				input.unused0 = 0;
				input.timestamp_ms = 0; // todo: get timestamp
				input.pad = e;
				((window_impl*)user)->trigger(input);
			
			}, this);
		}
	}
}

void window_impl::flush_messages(bool wait_for_next)
{
	while (hWnd)
	{
		HACCEL hAccel = nullptr;
		if (CurrentHotkeySet >= 0 && CurrentHotkeySet < (int)HotkeySets.size())
			hAccel = (HACCEL)HotkeySets[CurrentHotkeySet];

		int r = oWinDispatchMessage(hWnd, hAccel, wait_for_next);
		if (r <= 0)
			break;

		// only wait on the first call for the function, then flush all events and
		// exit out to allow an external loop to take place that should then cycle
		// around to this again, where the wait_for_next will be respected once more.
		wait_for_next = false;
	}
}

void window_impl::quit()
{
	PostMessage(hWnd, oWM_QUIT, 0, 0);
	//dispatch_internal([=] { PostQuitMessage(0); });
};

void window_impl::trigger_generic_event(event_type::value _Event, window_shape* _pShape)
{
	window_shape s;

	if (_Event != event_type::closed)
		s = oWinGetShape(hWnd);

	shape_event e((window_handle)hWnd, _Event, s);
	EventHooks.visit(e);
	if (_pShape)
		*_pShape = e.shape;
}

void window_impl::set_cursor()
{
	cursor::set(nullptr, ClientCursorState);
}

bool window_impl::handle_lifetime(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult)
{
	switch (msg)
	{
		case WM_CREATE:
		{
			CREATESTRUCTA* cs = (CREATESTRUCTA*)lparam;
			oWIN_CREATESTRUCT* wcs = (oWIN_CREATESTRUCT*)cs->lpCreateParams;
			const init_t* pInit = (const init_t*)wcs->pInit;
			oAssert(pInit, "invalid init struct");

			for (uint8_t i = 0; i < countof(pads); i++)
				pads[i] = pad_t(i);

			// this is a bit dangerous because it's not really true this hWnd is 
			// ready for use, but we need to expose it consistently as a valid 
			// return value from native_handle() so it can be accessed from 
			// event_type::creating, where it's known to be only semi-ready.
			hWnd = hwnd;

			create_event e((window_handle)hwnd
				, (statusbar_handle)oWinGetStatusBar(hwnd)
				, (menu_handle)oWinGetMenu(hwnd)
				, wcs->Shape, pInit->create_user_data);
			EventHooks.visit(e);
			break;
		}
	
		case WM_CLOSE:
		{
			// Don't allow DefWindowProc to destroy the window, put it all on client 
			// code.
			shape_event e((window_handle)hWnd, event_type::closing, oWinGetShape(hWnd));
			EventHooks.visit(e);
			*out_lresult = 0;
			return true;
		}

		case oWM_DESTROY:
			DestroyWindow(hwnd);
			break;

		case WM_DESTROY:
		{
			trigger_generic_event(event_type::closed);
			hWnd = nullptr;

			int n = (int)HotkeySets.size();
			for (int i = 0; i < n; i++)
				HotkeySets.remove(i);

			*out_lresult = 0;
			Destroyed.set();
			return true;
		}

		default:
			break;
	}

	return false;
}

bool window_impl::handle_misc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult)
{
	switch (msg)
	{
		case WM_SETCURSOR:
			if (LOWORD(lparam) == HTCLIENT)
			{
				set_cursor();
				*out_lresult = TRUE;
				return true;
			}
			break;

		case WM_GETDLGCODE:
			// this window is not a sub-class of any window, so allow all keys to come 
			// through.
			*out_lresult = DLGC_WANTALLKEYS;
			return true;

		case WM_ACTIVATE:
			for (uint32_t i = 0; i < countof(pads); i++)
				pads[i].enable(wparam != WA_INACTIVE && AllowControllers);
			trigger_generic_event((wparam == WA_INACTIVE) ? event_type::deactivated : event_type::activated);
			break;

		// All these should be treated the same if there's any reason to override 
		// painting.
		case WM_PAINT: case WM_PRINT: case WM_PRINTCLIENT:
			trigger_generic_event(event_type::paint);
			break;

		case WM_DISPLAYCHANGE:
			trigger_generic_event(event_type::display_changed);
			break;

		case oWM_DISPATCH:
		{
			std::function<void()>* pTask = (std::function<void()>*)lparam;
			(*pTask)();
			delete_object(pTask);
			*out_lresult = 0;
			return true;
		}

		case WM_TIMER:
		{
			if (wparam == (WPARAM)(&SortOrder))
				oWinSetFocus(hwnd);
			else if (wparam == (WPARAM)pads)
				handle_controllers();
			else
			{
				timer_event e((window_handle)hwnd, (uintptr_t)wparam);
				EventHooks.visit(e);
			}
			*out_lresult = 1;
			return true;
		}

		default:
			break;
	}

	return false;
}

bool window_impl::handle_sizing(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult)
{
	*out_lresult = 0;

	switch (msg)
	{
		case WM_WINDOWPOSCHANGING:
			trigger_generic_event(event_type::moving);
			return true;

		case WM_MOVE:
		{
			//oTrace("HWND 0x%x WM_MOVE: %dx%d", hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

			window_shape s;
			trigger_generic_event(event_type::moved, &s);
			PriorShape.client_position = s.client_position;
			return true;
		}

		case WM_SIZE:
		{
			//oTrace("WM_SIZE: %dx%d", GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));

			if (!oWinIsTempChange(hwnd))
			{
				shape_event e((window_handle)hWnd, event_type::sizing, PriorShape);
				EventHooks.visit(e);

				e.type = event_type::sized;
				e.shape = oWinGetShape(hwnd);
				EventHooks.visit(e);
				PriorShape = e.shape;
			}
			return true;
		}

		default:
			break;
	}

	return false;
}

bool window_impl::handle_input(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, LRESULT* out_lresult)
{
	*out_lresult = 0;

	input_t input;
	memset(&input, 0, sizeof(input));
	input.type = input_type::count;
	input.status = peripheral_status::full_power;
	input.unused0 = 0;
	input.timestamp_ms = (uint32_t)GetMessageTime();

	bool CheckDrag = false;
	
	if (int kb_result = handle_keyboard(hwnd, msg, wparam, lparam, &input.keypress))
	{
		if (keyboard && kb_result)
			keyboard->trigger(input.keypress);

		if (kb_result > 0)
		{
			input.type = input_type::keypress;
			CheckDrag = true;
		}
	}

	if (handle_mouse(hwnd, msg, wparam, lparam, CaptureType, &MouseContext, &input.mouse))
	{
		if (mouse)
			mouse->trigger(input.mouse);
		input.type = input_type::mouse;
		CheckDrag = input.mouse.type == mouse_event::press;
	}

	if (CheckDrag)
	{
		if (ClientDragToMove)
		{
			const mouse_button check = GetSystemMetrics(SM_SWAPBUTTON) ? mouse_button::right : mouse_button::left;

			if (input.mouse.pr.button == check)
			{
				if (input.mouse.pr.down)
					cursor::capture(hwnd, &MouseAtCaptureX, &MouseAtCaptureY);
				else
				{
					MouseAtCaptureX = MouseAtCaptureY = 0;
					cursor::capture(nullptr);
				}
			}
		}
	}

	else if (input.type == input_type::mouse)
	{
		// Ensure the cursor specified by the user remains valid within the client
		// area.
		if (cursor::captured() == hwnd)
		{
			RECT rClient;
			GetClientRect(hwnd, &rClient);
			POINT pt = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };
			if (PtInRect(&rClient, pt))
				set_cursor();
		}

		// Handle dragging in the client area
		if (ClientDragToMove && MouseAtCaptureX != oDEFAULT)
		{
			POINT p;
			p.x = GET_X_LPARAM(lparam);
			p.y = GET_Y_LPARAM(lparam);
			ClientToScreen(hwnd, &p);
			window_shape Shape;
			Shape.client_position = int2(p.x, p.y) - int2(MouseAtCaptureX, MouseAtCaptureY);
			try { oWinSetShape(hwnd, Shape); }
			catch (std::exception& e) { e; oTraceA("ERROR: oWinSetShape: %s", e.what()); }
		}
	}

	else if (handle_hotkey(hwnd, msg, wparam, lparam, &input.hotkey))
		input.type = input_type::hotkey;

	else if (handle_menu(hwnd, msg, wparam, lparam, &input.menu))
		input.type = input_type::menu;
	
	else if (handle_control(hwnd, msg, wparam, lparam, &input.control))
		input.type = input_type::control;

	else if (handle_touch(msg, wparam, lparam, &input.touch))
		input.type = input_type::touch;

	else if (handle_skeleton(msg, wparam, lparam, &input.skeleton))
		input.type = input_type::skeleton;

	if (input.type != input_type::count)
	{
		InputHooks.visit(input);
		return true;
	}

	switch (msg)
	{
		case WM_CANCELMODE:
		{
			if (cursor::captured() == hwnd)
			{
				cursor::capture(nullptr);
				trigger_generic_event(event_type::lost_capture);
			}
			break;
		}

		case WM_CAPTURECHANGED:
		{
			if ((HWND)lparam != hwnd)
			{
				if (cursor::captured() == hwnd)
					trigger_generic_event(event_type::lost_capture);
			}
			break;
		}
	
		// WM_INPUT_DEVICE_CHANGE is parsed into a more reasonable messaging system
		// so use oWM_INPUT_DEVICE_CHANGE as its proxy.
		case oWM_INPUT_DEVICE_CHANGE:
		{
			input_device_event e((window_handle)hwnd
				, input_type(LOWORD(wparam))
				, peripheral_status(HIWORD(wparam))
				, (const char*)lparam);
			EventHooks.visit(e);
			return true;
		}

		case WM_DROPFILES:
		{
			int2 p(0, 0);
			DragQueryPoint((HDROP)wparam, (POINT*)&p);
			const int NumPaths = DragQueryFileA((HDROP)wparam, ~0u, nullptr, 0); 
			path_string* pPaths = new path_string[NumPaths];
			for (int i = 0; i < NumPaths; i++)
				DragQueryFileA((HDROP)wparam, i, const_cast<char*>(pPaths[i].c_str()), (uint32_t)pPaths[i].capacity());
			DragFinish((HDROP)wparam);

			drop_event e((window_handle)hwnd, pPaths, NumPaths, p);
			EventHooks.visit(e);
			delete [] pPaths;
			return true;
		}

		default:
			break;
	}

	return false;
}

LRESULT window_impl::WndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if ((Debug || kForceDebug))
	{
		xlstring s;
		oTrace("%s", windows::parse_wm_message(s, s.capacity(), hwnd, msg, wparam, lparam));
	}

	LRESULT lResult = -1;
	if (handle_input(hwnd, msg, wparam, lparam, &lResult))
		return lResult;

	if (handle_sizing(hwnd, msg, wparam, lparam, &lResult))
		return lResult;

	if (handle_misc(hwnd, msg, wparam, lparam, &lResult))
		return lResult;

	if (handle_lifetime(hwnd, msg, wparam, lparam, &lResult))
		return lResult;

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

}
