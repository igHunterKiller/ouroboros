// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oGUI/menu.h>
#include <oGUI/msgbox.h>
#include <oCore/color.h>
#include <oCore/timer.h>
#include <oGUI/window.h>
#include <oGUI/Windows/win_common_dialog.h>
#include <oGUI/Windows/win_gdi_draw.h>
#include <oGUI/Windows/oWinWindowing.h>
#include <oSystem/system.h>
#include <oSystem/windows/win_error.h>

using namespace ouro::gui;

using namespace ouro;

static const bool kInteractiveMode = false;

class oWindowUITest
{
public:
	oWindowUITest(bool* _pSuccess);

	window* GetWindow() { return Window.get(); }
	bool GetRunning() const { return Running; }

private:
	std::shared_ptr<window> Window;
	bool Running;

	enum
	{
		MENU_FILE_OPEN,
		MENU_FILE_SAVE,
		MENU_FILE_SAVEAS,
		MENU_FILE_EXIT,
		MENU_EDIT_CUT,
		MENU_EDIT_COPY,
		MENU_EDIT_PASTE,
		MENU_EDIT_COLOR,
		MENU_EDIT_FONT,
		MENU_VIEW_SOLID,
		MENU_VIEW_WIREFRAME,
		MENU_VIEW_SHOW_STATUSBAR,
		MENU_HELP_ABOUT,
	};

	ouro::menu_handle hFileMenu;
	ouro::menu_handle hEditMenu;
	ouro::menu_handle hViewMenu;
	ouro::menu_handle hStatusBarMenu;
	ouro::menu_handle hHelpMenu;

	void InitMenu(ouro::menu_handle _hWindowMenu);
	void InitVCRControls(HWND _hParent, const int2& _Position);

	void OnCreate(HWND _hWnd, ouro::menu_handle _hMenu);
	void OnMenuCommand(HWND _hWnd, int _MenuID);

	enum
	{
		ID_PUSHME,
		ID_FLOATBOX,
		ID_FLOATBOX_SPINNER,
		ID_EASY,

		ID_BIG_BACK,
		ID_BACK,
		ID_PLAY_PAUSE,
		ID_FORWARD,
		ID_BIG_FORWARD,

		ID_SLIDER,
		ID_SLIDER_SELECTABLE,

		ID_PROGRESSBAR,
		ID_MARQUEE,

		ID_COMBOBOX,
		ID_COMBOTEXTBOX,

		NUM_CONTROLS,
	};

	HWND Controls[NUM_CONTROLS];
	ouro::border_style::value BorderStyle;

	void EventHook(const window::basic_event& _Event);
	void InputHook(const ouro::input_t& _Input);
};

void oWindowUITest::EventHook(const window::basic_event& _Event)
{
	switch (_Event.type)
	{
		case ouro::event_type::sized:
			oTrace("NewClientSize = %dx%d%s", _Event.as_shape().shape.client_size.x, _Event.as_shape().shape.client_size.y, _Event.as_shape().shape.state == ouro::window_state::minimized ? " (minimized)" : "");
			break;
		case ouro::event_type::creating:
		{
			OnCreate((HWND)_Event.as_create().window, _Event.as_create().menu);
			break;
		}

		default:
			break;
	}
}

void oWindowUITest::InputHook(const ouro::input_t& _Input)
{
	switch (_Input.type)
	{
		case ouro::input_type::control:
		{
			switch (_Input.control.status)
			{
				case ouro::control_status::activated:
				{
					ouro::lstring text;
					oWinControlGetText(text, (HWND)_Input.control.window);
					oTrace("Action %s \"%s\" code=%d", ouro::as_string(oWinControlGetType((HWND)_Input.control.window)), text.c_str(), _Input.control.native_status);
					break;
				}
				default:
					break;
			}
			break;
		}

		case ouro::input_type::menu:
			OnMenuCommand((HWND)_Input.menu.window, _Input.menu.id);
			break;
		case ouro::input_type::hotkey:
			OnMenuCommand((HWND)_Input.hotkey.window, _Input.hotkey.id);
			break;

		case ouro::input_type::mouse:
		{
			if (kInteractiveMode)
				Window->set_status_text(0, "Cursor: %dx%d", _Input.mouse.mv.x, _Input.mouse.mv.y);

			break;
		}

		default:
			break;
	}
}

void oWindowUITest::InitMenu(ouro::menu_handle _hWindowMenu)
{
	hFileMenu = menu::make_menu();
	hEditMenu = menu::make_menu();
	hViewMenu = menu::make_menu();
	hStatusBarMenu = menu::make_menu();
	hHelpMenu = menu::make_menu();

	menu::append_submenu(_hWindowMenu, hFileMenu, "&File");
		menu::append_item(hFileMenu, MENU_FILE_OPEN, "&Open...");
		menu::append_item(hFileMenu, MENU_FILE_SAVE, "&Save");
		menu::append_item(hFileMenu, MENU_FILE_SAVEAS, "Save &As...");
		menu::append_separator(hFileMenu);
		menu::append_item(hFileMenu, MENU_FILE_EXIT, "E&xit");
	menu::append_submenu(_hWindowMenu, hEditMenu, "&Edit");
		menu::append_item(hEditMenu, MENU_EDIT_CUT, "Cu&t");
		menu::append_item(hEditMenu, MENU_EDIT_COPY, "&Copy");
		menu::append_item(hEditMenu, MENU_EDIT_PASTE, "&Paste");
		menu::append_separator(hEditMenu);
		menu::append_item(hEditMenu, MENU_EDIT_COLOR, "Co&lor");
		menu::append_item(hEditMenu, MENU_EDIT_FONT, "&Font");
	menu::append_submenu(_hWindowMenu, hViewMenu, "&View");
		menu::append_item(hViewMenu, MENU_VIEW_SOLID, "&Solid\tCtrl+S");
		menu::check(hViewMenu, MENU_VIEW_SOLID, true);
		menu::append_item(hViewMenu, MENU_VIEW_WIREFRAME, "&Wireframe\tCtrl+W");
		menu::append_separator(hViewMenu);
		menu::append_item(hViewMenu, MENU_VIEW_SHOW_STATUSBAR, "&Show Statusbar");
		menu::check(hViewMenu, MENU_VIEW_SHOW_STATUSBAR, true);
	menu::append_submenu(_hWindowMenu, hHelpMenu, "&Help");
		menu::append_item(hHelpMenu, MENU_HELP_ABOUT, "&About...");
}

void oWindowUITest::InitVCRControls(HWND _hParent, const int2& _Position)
{
	static const int2 kButtonSize = int2(25,25);
	static const int2 kButtonSpacing = int2(27,0);

	ouro::font_info fd;
	fd.name = "Webdings";
	fd.point_size = 12;
	fd.antialiased = false; // ClearType is non-deterministic, so disable it for screen-compare purposes
	HFONT hWebdings = windows::gdi::make_font(fd);

	ouro::control_info b;
	b.parent = (ouro::window_handle)_hParent;
	b.type = ouro::control_type::button;
	b.size = kButtonSize;
	
	b.text = "9";
	b.position = _Position;
	b.id = ID_BIG_BACK;
	HWND hWnd = oWinControlCreate(b);
	oWinControlSetFont(hWnd, hWebdings);

	b.text = "7";
	b.position += kButtonSpacing;
	b.id = ID_BACK;
	hWnd = oWinControlCreate(b);
	oWinControlSetFont(hWnd, hWebdings);

	b.text = "4";
	b.position += kButtonSpacing;
	b.id = ID_PLAY_PAUSE;
	hWnd = oWinControlCreate(b);
	oWinControlSetFont(hWnd, hWebdings);

	b.text = "8";
	b.position += kButtonSpacing;
	b.id = ID_FORWARD;
	hWnd = oWinControlCreate(b);
	oWinControlSetFont(hWnd, hWebdings);

	b.text = ":";
	b.position += kButtonSpacing;
	b.id = ID_BIG_FORWARD;
	hWnd = oWinControlCreate(b);
	oWinControlSetFont(hWnd, hWebdings);
}

oWindowUITest::oWindowUITest(bool* _pSuccess)
	: BorderStyle(ouro::border_style::sunken)
	, Running(true)
{
	*_pSuccess = false;

	window::init_t i;
	i.title = "TESTWindowControls";
	i.on_event = std::bind(&oWindowUITest::EventHook, this, std::placeholders::_1);
	i.on_input = std::bind(&oWindowUITest::InputHook, this, std::placeholders::_1);
	i.shape.state = ouro::window_state::restored;
	i.shape.style = ouro::window_style::sizable_with_menu_and_statusbar;
	i.shape.client_size = int2(640,480);
	Window = window::make(i);

	int Widths[2] = { 100, -1 };
	Window->set_num_status_sections(Widths);

	// Disable anti-aliasing since on Windows ClearType seems to be non-deterministic
	Window->dispatch([&]
	{
		HWND hWnd = (HWND)Window->native_handle();
		ouro::font_info fi = windows::gdi::get_font_info(oWinGetFont(hWnd));
		fi.antialiased = false;
		HFONT hNew = windows::gdi::make_font(fi);
		oWinSetFont(hWnd, hNew);
	});

	Window->set_status_text(0, "OK");
	Window->set_status_text(1, "Solid");

	ouro::hotkey HotKeys[] = 
	{
		{ ouro::key::w, key_modifier::ctrl, MENU_VIEW_WIREFRAME },
		{ ouro::key::s, key_modifier::ctrl, MENU_VIEW_SOLID },
	};

	Window->new_hotkeys(HotKeys);

	*_pSuccess = true;
}

void oWindowUITest::OnCreate(HWND _hWnd, ouro::menu_handle _hMenu)
{
	InitMenu(_hMenu);
	InitVCRControls(_hWnd, int2(150, 30));

	ouro::control_info ButtonDesc;
	ButtonDesc.parent = (ouro::window_handle)_hWnd;
	ButtonDesc.type = ouro::control_type::button;
	ButtonDesc.text = "Push Me";
	ButtonDesc.position = int2(10,10);
	ButtonDesc.size = int2(100,20);
	ButtonDesc.id = ID_PUSHME;
	Controls[ID_PUSHME] = oWinControlCreate(ButtonDesc);

	ouro::control_info TextDesc;
	TextDesc.parent = (ouro::window_handle)_hWnd;
	TextDesc.type = ouro::control_type::floatbox;
	TextDesc.text = "1.00";
	TextDesc.position = int2(50,50);
	TextDesc.size = int2(75,20);
	TextDesc.id = ID_FLOATBOX;
	Controls[ID_FLOATBOX] = oWinControlCreate(TextDesc);

	oWinControlSetValue(Controls[ID_FLOATBOX], 1.234f);

	oTrace("About to test an invalid case, an exception may be caught by the debugger. CONTINUE.");
	try
	{
		oWinControlSetText(Controls[ID_FLOATBOX], "Error!"); // should not show up
	}
	catch (std::exception&) {}

	TextDesc.type = ouro::control_type::floatbox_spinner;
	TextDesc.position.y += 40;
	TextDesc.id = ID_FLOATBOX_SPINNER;
	Controls[ID_FLOATBOX_SPINNER] = oWinControlCreate(TextDesc);
	oVB(Controls[ID_FLOATBOX_SPINNER]);

	ButtonDesc.text = "Easy";
	ButtonDesc.position = int2(10,450);
	ButtonDesc.size = int2(100,20);
	ButtonDesc.id = ID_EASY;
	oWinControlCreate(ButtonDesc);

	ouro::control_info SliderDesc;
	SliderDesc.parent = (ouro::window_handle)_hWnd;
	SliderDesc.type = ouro::control_type::slider;
	SliderDesc.text = "Slider";
	SliderDesc.position = int2(150,60);
	SliderDesc.size = int2(140,25);
	SliderDesc.id = ID_SLIDER;
	Controls[ID_SLIDER] = oWinControlCreate(SliderDesc);

	oCHECK(oWinControlSetRange(Controls[ID_SLIDER], 20, 120), "");
	oCHECK(oWinControlSetRangePosition(Controls[ID_SLIDER], 70), "");

	SliderDesc.type = ouro::control_type::slider_selectable;
	SliderDesc.text = "SliderSelectable";
	SliderDesc.position = int2(150,90);
	SliderDesc.id = ID_SLIDER_SELECTABLE;
	Controls[ID_SLIDER_SELECTABLE] = oWinControlCreate(SliderDesc);

	oCHECK(oWinControlSetRange(Controls[ID_SLIDER_SELECTABLE], 0, 100), "");
	oCHECK(oWinControlSelect(Controls[ID_SLIDER_SELECTABLE], 10, 50), "");
	oCHECK(oWinControlSetRangePosition(Controls[ID_SLIDER_SELECTABLE], 30), "");

	ouro::control_info ProgressBarDesc;
	ProgressBarDesc.parent = (ouro::window_handle)_hWnd;
	ProgressBarDesc.type = ouro::control_type::progressbar;
	ProgressBarDesc.text = "ProgressBar";
	ProgressBarDesc.position = int2(150,120);
	ProgressBarDesc.size = int2(150,30);
	ProgressBarDesc.id = ID_PROGRESSBAR;
	Controls[ID_PROGRESSBAR] = oWinControlCreate(ProgressBarDesc);
	
	oCHECK(oWinControlSetRange(Controls[ID_PROGRESSBAR], 20, 30), "");
	oCHECK(oWinControlSetRangePosition(Controls[ID_PROGRESSBAR], 25), "");
	oCHECK(oWinControlSetErrorState(Controls[ID_PROGRESSBAR], true), "");

	// Marquee animates itself, making it a poor element of an automated image
	// test, so disable this for screen grabs, but show it's working in 
	// interactive mode
	if (kInteractiveMode)
	{
		ProgressBarDesc.type = ouro::control_type::progressbar_unknown;
		ProgressBarDesc.text = "Marquee";
		ProgressBarDesc.position = int2(150,160);
		ProgressBarDesc.id = ID_MARQUEE;
		Controls[ID_MARQUEE] = oWinControlCreate(ProgressBarDesc);
	}

	ouro::control_info CB;
	CB.parent = (ouro::window_handle)_hWnd;
	CB.type = ouro::control_type::combobox;
	CB.text = "Init1|Init2|Init3";
	CB.position = int2(50,120);
	CB.size = int2(80,30);
	CB.id = ID_COMBOBOX;
	Controls[ID_COMBOBOX] = oWinControlCreate(CB);

	oWinControlInsertSubItem(Controls[ID_COMBOBOX], "After3", -1);
	oWinControlInsertSubItem(Controls[ID_COMBOBOX], "Btwn2And3", 2);

	CB.type = ouro::control_type::combotextbox;
	CB.position = int2(50,160);
	CB.id = ID_COMBOTEXTBOX;
	Controls[ID_COMBOTEXTBOX] = oWinControlCreate(CB);

	oWinControlInsertSubItem(Controls[ID_COMBOTEXTBOX], "After3", -1);
	oWinControlInsertSubItem(Controls[ID_COMBOTEXTBOX], "Btwn2And3", 2);
	oWinControlSetText(Controls[ID_COMBOTEXTBOX], "Select...");
}

void oWindowUITest::OnMenuCommand(HWND _hWnd, int _MenuID)
{
	bool StatusBarChanged = false;

	switch (_MenuID)
	{
		case MENU_FILE_OPEN:
		{
			ouro::path_t path;
			if (windows::common_dialog::open_path(path, "TESTWindowUI", "Source Files|*.cpp|Header Files|*.h", _hWnd))
				oTrace("Open %s", path.c_str());
			else
				oTrace("Open dialog canceled.");

			ouro::msgbox(ouro::msg_type::info, nullptr, "TESTWindowUI", "User selected path:\n\t%s", path.c_str());
			break;
		}
		case MENU_FILE_SAVE:
			break;
		case MENU_FILE_SAVEAS:
		{
			ouro::path_t path;
			if (windows::common_dialog::save_path(path, "TESTWindowUI", "Source Files|*.cpp|Header Files|*.h", _hWnd))
				oTrace("SaveAs %s", path.c_str());
			else
				oTrace("SaveAs dialog canceled.");

			ouro::msgbox(ouro::msg_type::info, nullptr, "TESTWindowUI", "User selected path:\n\t%s", path.c_str());
			break;
		}
		case MENU_FILE_EXIT:
			Running = false;
			break;
		case MENU_EDIT_CUT:
			break;
		case MENU_EDIT_COPY:
			break;
		case MENU_EDIT_PASTE:
			break;
		case MENU_EDIT_COLOR:
		{
			uint32_t c = ouro::color::red;

			if (!windows::common_dialog::pick_color(&c, _hWnd))
				ouro::msgbox(ouro::msg_type::info, nullptr, "TESTWindowUI", "No color!");

			else
			{
				argb_channels ch;
				ch.argb = c;
				ouro::msgbox(ouro::msg_type::info, nullptr, "TESTWindowUI", "Color: %d,%d,%d", ch.r, ch.g, ch.b);
			}
			
			break;
		}
		case MENU_EDIT_FONT:
		{
			uint32_t c = ouro::color::red;
			LOGFONT lf = {0};
			GetObject((HFONT)GetStockObject(DEFAULT_GUI_FONT), sizeof(LOGFONT), &lf);
			if (!windows::common_dialog::pick_font(&lf, &c, _hWnd))
				ouro::msgbox(ouro::msg_type::info, nullptr, "TESTWindowUI", "No color!");

			else
			{
				argb_channels ch;
				ch.argb = c;
				ouro::msgbox(ouro::msg_type::info, nullptr, "TESTWindowUI", "Color: %d,%d,%d", ch.r, ch.g, ch.b);
			}
			
			break;
		}
		case MENU_VIEW_SOLID:
			menu::check(hViewMenu, MENU_VIEW_SOLID, true);
			menu::check(hViewMenu, MENU_VIEW_WIREFRAME, false);
			menu::enable(hFileMenu, MENU_FILE_EXIT);
			Window->set_status_text(1, "Solid");
			break;
		case MENU_VIEW_WIREFRAME:
			menu::check(hViewMenu, MENU_VIEW_SOLID, false);
			menu::check(hViewMenu, MENU_VIEW_WIREFRAME, true);
			menu::enable(hFileMenu, MENU_FILE_EXIT, false);
			Window->set_status_text(1, "Wireframe");
			break;
		case MENU_VIEW_SHOW_STATUSBAR:
		{
			bool NewState = !menu::checked(hViewMenu, MENU_VIEW_SHOW_STATUSBAR);
			menu::check(hViewMenu, MENU_VIEW_SHOW_STATUSBAR, NewState);
			ouro::window_style::value s;
			s = NewState ? ouro::window_style::sizable_with_menu_and_statusbar : ouro::window_style::sizable_with_menu;
			Window->style(s);
			break;
		}
		case MENU_HELP_ABOUT:
			break;
		default:
			break;
	}

	if (menu::checked(hViewMenu, MENU_VIEW_SOLID))
		oTrace("View Solid");
	else if (menu::checked(hViewMenu, MENU_VIEW_WIREFRAME))
		oTrace("View Wireframe");
	else
		oTrace("View Nothing");

	oTrace("Exit is %sabled", menu::enabled(hFileMenu, MENU_FILE_EXIT) ? "en" : "dis");
}

oTEST(oGUI_WindowControls)
{
	if (ouro::system::is_remote_session())
		throw skip_test("Detected remote session: differing text anti-aliasing will cause bad image compares");

	bool success = false;
	oWindowUITest test(&success);
	oCHECK(success, "failed to construct test window");

	double WaitForSettle = ouro::timer::now() + 1.0;
	do
	{
		test.GetWindow()->flush_messages();

	} while (ouro::timer::now() < WaitForSettle);

	ouro::future<ouro::surface::image> snapshot = test.GetWindow()->snapshot();
		
	do
	{
		test.GetWindow()->flush_messages();
		
	} while ((kInteractiveMode && test.GetRunning()) || !snapshot.is_ready());

	ouro::surface::image s = snapshot.get();
	srv.check(s, 0);
}
