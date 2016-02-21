// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oSystem/filesystem.h>
#include <oSystem/peripherals.h>
#include <oSystem/reporting.h>
#include <oGUI/Windows/win_gdi_bitmap.h>
#include <oGUI/menu.h>
#include <oGUI/enum_radio_handler.h>
#include <oGUI/msgbox.h>
#include <oGUI/msgbox_reporting.h>
#include <oGUI/window.h>
#include "resource.h"
#include <atomic>

#include <oSystem/filesystem_monitor.h>

#include "../about_ouroboros.h"

#include "app.h"

using namespace ouro;
using namespace ouro::gui;
using namespace ouro::windows::gdi;

enum oWMENU
{
	oWMENU_FILE,
	oWMENU_EDIT,
	oWMENU_VIEW,
	oWMENU_VIEW_STYLE,
	oWMENU_VIEW_STATE,
	oWMENU_HELP,
	oWMENU_COUNT,
	oWMENU_TOPLEVEL,
};

struct oWMENU_HIER
{
	oWMENU Parent; // use oWMENU_TOPLEVEL for root parent menu
	oWMENU Menu;
	const char* Name;
};

static oWMENU_HIER sMenuHier[] = 
{
	{ oWMENU_TOPLEVEL, oWMENU_FILE, "&File" },
	{ oWMENU_TOPLEVEL, oWMENU_EDIT, "&Edit" },
	{ oWMENU_TOPLEVEL, oWMENU_VIEW, "&View" },
	{ oWMENU_VIEW, oWMENU_VIEW_STYLE, "Border Style" },
	{ oWMENU_VIEW, oWMENU_VIEW_STATE, "&Window State" },
	{ oWMENU_TOPLEVEL, oWMENU_HELP, "&Help" },
};
match_array(sMenuHier, oWMENU_COUNT);

enum oWMI // menuitems
{
	oWMI_FILE_EXIT,

	oWMI_VIEW_STYLE_FIRST,
	oWMI_VIEW_STYLE_LAST = oWMI_VIEW_STYLE_FIRST + (int)window_style::count - 1,

	oWMI_VIEW_STATE_FIRST,
	oWMI_VIEW_STATE_LAST = oWMI_VIEW_STATE_FIRST + (int)window_state::count - 1,

	oWMI_HELP_ABOUT,
};

enum oWHK // hotkeys
{
	oWHK_DEFAULT_STYLE,
	oWHK_TOGGLE_FULLSCREEN,
};

hotkey HotKeys[] =
{
	{ key::f3, key_modifier::none, oWHK_DEFAULT_STYLE },
	{ key::f11, key_modifier::none, oWHK_TOGGLE_FULLSCREEN },
};

struct oWindowTestAppPulseContext
{
	oWindowTestAppPulseContext() : Count(0) {}
	unsigned int Count;
};

enum oWCTL // controls
{
	oWCTL_EASY_BUTTON,
};

class oWindowTestApp
{
public:
	oWindowTestApp();

	void Run();

private:
	std::shared_ptr<window> Window;
	std::shared_ptr<about> About;
	menu_handle Menus[oWMENU_COUNT];
	oWindowTestAppPulseContext PulseContext;
	menu::enum_radio_handler RadioHandler; 
	window_state PreFullscreenState;
	bool Running;

	mouse_t Mouse;
	keyboard_t Keyboard;

	// This gets deleted by parent window automatically.
	HWND hButton;

	std::shared_ptr<filesystem::monitor> DirWatcher;

private:
	void InputHook(const input_t& _Input);
	void EventHook(const window::basic_event& _Event);
	void CreateMenu(const window::create_event& _CreateEvent);
	void CreateControls(const window::create_event& _CreateEvent);
	void CheckState(window_state _State);
	void CheckStyle(window_style _Style);
	static void OnDirectoryEvent(filesystem::file_event _Event, const path_t& _Path, void* _User);
};

oWindowTestApp::oWindowTestApp()
	: Running(true)
	, PreFullscreenState(window_state::hidden)
	, hButton(nullptr)
{
	{
		filesystem::monitor::info i;
		i.accessibility_poll_rate_ms = 2000;
		i.accessibility_timeout_ms = 5000;

		DirWatcher = filesystem::monitor::make(i, &oWindowTestApp::OnDirectoryEvent, this);
	}

	{
		path_t watched = filesystem::desktop_path() / "test/";
		try { DirWatcher->watch(watched, 64 * 1024, true); }
		catch (std::exception& e) { e; oTraceA("Cannot watch %s: %s", watched.c_str(), e.what()); }
	}

	Keyboard.initialize();
	Mouse.initialize();

	{
		window::init_t i;
		i.title = "oWindowTestApp";
		i.icon = (icon_handle)load_icon(IDI_APPICON);
		i.keyboard = &Keyboard;
		i.mouse = &Mouse;
		i.on_input = std::bind(&oWindowTestApp::InputHook, this, std::placeholders::_1);
		i.on_event = std::bind(&oWindowTestApp::EventHook, this, std::placeholders::_1);
		i.shape.client_size = int2(320, 240);
		i.shape.state = window_state::hidden;
		i.shape.style = window_style::sizable_with_menu_and_statusbar;
		i.allow_controller = true;
		i.alt_f4_closes = true;
		i.cursor = mouse_cursor::hand;
		i.debug = false;

		Window = window::make(i);
	}

	Window->new_hotkeys(HotKeys);

	const int sSections[] = { 85, 120, 100 };
	Window->set_num_status_sections(sSections, countof(sSections));
	Window->set_status_text(0, "Timer: 0");
	Window->set_status_text(1, "F3 for default style");
	Window->set_status_text(2, "Easy: 0");

	Window->start_timer((uintptr_t)&PulseContext, 2000);

	Window->show();
}

void oWindowTestApp::CreateMenu(const window::create_event& _CreateEvent)
{
	for (auto& m : Menus)
		m = menu::make_menu();

	for (const auto& h : sMenuHier)
	{
		menu::append_submenu(
			h.Parent == oWMENU_TOPLEVEL ? _CreateEvent.menu : Menus[h.Parent]
		, Menus[h.Menu], h.Name);
	}

	menu::append_item(Menus[oWMENU_FILE], oWMI_FILE_EXIT, "E&xit\tAlt+F4");

	menu::append_enum_items(window_style::count, Menus[oWMENU_VIEW_STYLE], oWMI_VIEW_STYLE_FIRST, oWMI_VIEW_STYLE_LAST, _CreateEvent.shape.style);
	RadioHandler.add(Menus[oWMENU_VIEW_STYLE], oWMI_VIEW_STYLE_FIRST, oWMI_VIEW_STYLE_LAST, [=](int _BorderStyle)
	{
		Window->style((window_style)_BorderStyle);
	});

	menu::append_enum_items(window_state::count, Menus[oWMENU_VIEW_STATE], oWMI_VIEW_STATE_FIRST, oWMI_VIEW_STATE_LAST, _CreateEvent.shape.state);
	RadioHandler.add(Menus[oWMENU_VIEW_STATE], oWMI_VIEW_STATE_FIRST, oWMI_VIEW_STATE_LAST, [=](int _State)
	{
		Window->show((window_state)_State);
	});

	menu::append_item(Menus[oWMENU_HELP], oWMI_HELP_ABOUT, "&About...");
}

void oWindowTestApp::CreateControls(const window::create_event& _CreateEvent)
{
	control_info d;
	d.parent = _CreateEvent.window;
	d.type = control_type::button;
	d.text = "&Easy";
	d.position = int2(20, 20);
	d.size = int2(80, 20);
	d.id = oWCTL_EASY_BUTTON;
	d.starts_new_group = true;
	hButton = oWinControlCreate(d);
}

void oWindowTestApp::CheckState(window_state _State)
{
	menu::check_radio(Menus[oWMENU_VIEW_STATE]
	, oWMI_VIEW_STATE_FIRST, oWMI_VIEW_STATE_LAST, oWMI_VIEW_STATE_FIRST + (int)_State);
}

void oWindowTestApp::CheckStyle(window_style _Style)
{
	menu::check_radio(Menus[oWMENU_VIEW_STYLE]
	, oWMI_VIEW_STYLE_FIRST, oWMI_VIEW_STYLE_LAST, oWMI_VIEW_STYLE_FIRST + (int)_Style);
}

void oWindowTestApp::OnDirectoryEvent(filesystem::file_event _Event, const path_t& _Path, void* _User)
{
	static std::atomic<int> counter = 0;

	if (_Event == filesystem::file_event::added && !filesystem::is_directory(_Path))
	{
		int old = counter++;
		oTrace("%s: %s (%d)", as_string(_Event), _Path.c_str(), old + 1);
	}

	else
	{
		oTrace("%s: %s", as_string(_Event), _Path.c_str());
	}
}

void oWindowTestApp::EventHook(const window::basic_event& _Event)
{
	switch (_Event.type)
	{
		case event_type::timer:
			if (_Event.as_timer().context == (uintptr_t)&PulseContext)
			{
				PulseContext.Count++;
				Window->set_status_text(0, "Timer: %d", PulseContext.Count);
			}
			else
				oTrace("event_type::timer");
			break;
		case event_type::activated:
			oTrace("event_type::activated");
			break;
		case event_type::deactivated:
			oTrace("event_type::deactivated");
			break;
		case event_type::creating:
		{
			oTrace("event_type::creating");
			CreateMenu(_Event.as_create());
			CreateControls(_Event.as_create());
			break;
		}
		case event_type::paint:
			//oTrace("event_type::paint");
			break;
		case event_type::display_changed:
			oTrace("event_type::display_changed");
			break;
		case event_type::moving:
			oTrace("event_type::moving");
			break;
		case event_type::moved:
			oTrace("event_type::moved %dx%d", _Event.as_shape().shape.client_position.x, _Event.as_shape().shape.client_position.y);
			break;
		case event_type::sizing:
			oTrace("event_type::sizing %s %dx%d", as_string(_Event.as_shape().shape.state), _Event.as_shape().shape.client_size.x, _Event.as_shape().shape.client_size.y);
			break;
		case event_type::sized:
		{
			oTrace("event_type::sized %s %dx%d", as_string(_Event.as_shape().shape.state), _Event.as_shape().shape.client_size.x, _Event.as_shape().shape.client_size.y);
			CheckState(_Event.as_shape().shape.state);
			CheckStyle(_Event.as_shape().shape.style);

			// center the button
			{
				RECT rParent, rButton;
				GetClientRect((HWND)_Event.window, &rParent);
				GetClientRect(hButton, &rButton);
				RECT Centered = oWinRect(resolve_rect(oRect(rParent), oRect(rButton), alignment::middle_center, false));
				SetWindowPos(hButton, nullptr, Centered.left, Centered.top, oWinRectW(Centered), oWinRectH(Centered), SWP_SHOWWINDOW);
			}
			break;
		}
		case event_type::closing:
			oTrace("event_type::closing");
			Running = false;
			Window->quit();
			break;
		case event_type::closed:
			oTrace("event_type::closed");
			break;
		case event_type::to_fullscreen:
			oTrace("event_type::to_fullscreen");
			break;
		case event_type::from_fullscreen:
			oTrace("event_type::from_fullscreen");
			break;
		case event_type::lost_capture:
			oTrace("event_type::lost_capture");
			break;
		case event_type::drop_files:
			oTrace("event_type::drop_files (at %d,%d starting with %s)", _Event.as_drop().client_drop_position.x, _Event.as_drop().client_drop_position.y, _Event.as_drop().paths[0]);
			break;
		case event_type::input_device_changed:
			oTrace("event_type::input_device_changed %s %s %s", as_string(_Event.as_input_device().type), as_string(_Event.as_input_device().status), _Event.as_input_device().instance_name);
			break;
		default: oThrow(std::errc::invalid_argument, "unexpected event %s", + as_string(_Event.type));
	}
}

void oWindowTestApp::InputHook(const input_t& _Input)
{
	switch (_Input.type)
	{
		case input_type::unknown:
			oTrace("input::unknown");
			break;

		case input_type::pad:
		{
			if (_Input.pad.type == pad_event::move)
			{
				oTrace("pad%u: left(%f, %f) right(%f, %f) ltrigger(%f) rtrigger(%f)", _Input.pad.index, _Input.pad.mv.lx, _Input.pad.mv.ly, _Input.pad.mv.rx, _Input.pad.mv.ry, _Input.pad.mv.ltrigger, _Input.pad.mv.rtrigger);
			}

			else
			{
				oTrace("%s %s", as_string(_Input.pad.pr.button), _Input.pad.pr.down ? "pressed" : "released");
			}

			break;
		}

		case input_type::menu:
			switch (_Input.menu.id)
			{
				case oWMI_FILE_EXIT:
					Running = false;
					Window->quit();
					break;
				case oWMI_HELP_ABOUT:
				{
					oDECLARE_ABOUT_INFO(i, load_icon(IDI_APPICON));
					if (!About)
						About = about::make(i);
					About->show_modal(Window);
					break;
				}
				default:
					RadioHandler.on_input(_Input);
					break;
			}
			break;

		case input_type::hotkey:
			switch (_Input.hotkey.id)
			{
				case oWHK_DEFAULT_STYLE:
				{
					if (Window->state() == window_state::fullscreen)
						Window->state(window_state::restored);
					Window->style(window_style::sizable_with_menu_and_statusbar);
					break;
				}
				case oWHK_TOGGLE_FULLSCREEN:
					if (Window->state() != window_state::fullscreen)
					{
						PreFullscreenState = Window->state();
						Window->state(window_state::fullscreen);
					}
					else
					{
						Window->state(PreFullscreenState);
					}
					break;
				default:
					oTrace("hotkey");
					break;
			}
			break;

		case input_type::control:
		{
			switch (_Input.control.status)
			{
				case control_status::activated:
					switch (_Input.control.id)
					{
						case oWCTL_EASY_BUTTON:
						{
							sstring text;
							Window->get_status_text(text, 2);
							int n = 0;
							from_string(&n, text.c_str() + 6);
							n++;
							Window->set_status_text(2, "Easy: %d", n);
							break;
						}
						default:
							oTrace("control_activated");
							break;
					}
					break;

				case control_status::deactivated:
					oTrace("deactivated");
					break;
				case control_status::selection_changing:
					oTrace("selection_changing");
					break;
				case control_status::selection_changed:
					oTrace("selection_changed");
					break;

				default:
					break;
			}

			break;
		}

		case input_type::keypress:
		{
			oTrace("keyboard key%s %s", _Input.keypress.down ? "down" : "up", as_string(_Input.keypress.key));
			break;
		}

		case input_type::mouse:
		{
			if (_Input.mouse.type == mouse_event::press)
			{
				oTrace("mouse %s %s", as_string(_Input.mouse.pr.button), _Input.mouse.pr.down ? "down" : "up");
			}

			else
				oTrace("mouse %d %d", _Input.mouse.mv.x, _Input.mouse.mv.y);

			break;
		}

		case input_type::skeleton:
		{
			oTrace("%s", as_string(_Input.skeleton.status));
			break;
		}

		default:
			break;
	}
}

void CheckInput(const keyboard_t& kb, const mouse_t& mse)
{
	for (int i = 0; i < (int)key::count; i++)
	{
		const key k = (key)i;

		if (kb.pressed(k))
			oTrace("%s pressed", as_string(k));
		else if (kb.released(k))
			oTrace("%s released", as_string(k));
	}
}

void oWindowTestApp::Run()
{
	while (Running)
	{
		Window->flush_messages(true);

		Mouse.update();
		Keyboard.update();

		//CheckInput(Keyboard, Mouse);
	}
}

#if 0
class oWindowTestApp2 : public app
{
public:
	struct pulse_context
	{
		pulse_context() : count(0) {}
		uint32_t count;
	};

	void initialize()
	{
		app::init_t app_init;
		app_init.msgbox_error_prompt = true;

		path watched = filesystem::desktop_path() / "test/";
		const char* watched_cstr = watched.c_str();

		app_init.watchdirs = &watched_cstr;
		app_init.num_watchdirs = 1;

		// window settings
		{
			window::init_t& i = app_init.window_init;
			i.title = "oWindowTestApp";
			i.icon = (icon_handle)load_icon(IDI_APPICON);
			i.on_event = std::bind(&oWindowTestApp2::EventHook, this, std::placeholders::_1);
			i.shape.client_size = int2(320, 240);
			i.shape.state = window_state::hidden;
			i.shape.style = window_style::sizable_with_menu_and_statusbar;
			i.allow_controller = true;
			i.alt_f4_closes = true;
			i.cursor_state = mouse_cursor::hand;
			i.debug = false;
		}

		app::initialize(app_init);

		window->set_hotkeys(HotKeys);

		const int sSections[] = { 85, 120, 100 };
		window->set_num_status_sections(sSections, countof(sSections));
		window->set_status_text(0, "Timer: 0");
		window->set_status_text(1, "F3 for default style");
		window->set_status_text(2, "Easy: 0");

		window->start_timer((uintptr_t)&pulsectx, 2000);

		window->show();
	}

	void EventHook(const window::basic_event& e) {}

	void on_dir_event(filesystem::file_event e, const path_t& p) override
	{
		static std::atomic<int> counter = 0;

		if (e == filesystem::file_event::added && !filesystem::is_directory(p))
		{
			int n = ++counter;
			oTrace("%s: %s (%d)", as_string(e), p.c_str(), n);
		}

		else
		{
			oTrace("%s: %s", as_string(e), p.c_str());
		}
	}
	
private:
	pulse_context pulsectx;
};
#endif

int main(int argc, const char* argv[])
{
#if 1
	reporting::set_prompter(prompt_msgbox);
	oWindowTestApp App;
	App.Run();
#else
	oWindowTestApp2 App;
	App.initialize();
	App.run();
	App.deinitialize();
#endif

	return 0;
}
