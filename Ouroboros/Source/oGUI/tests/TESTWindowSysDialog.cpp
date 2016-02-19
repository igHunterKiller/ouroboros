// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/unit_test.h>

#include <oCore/countof.h>
#include <oGUI/Windows/oWinRect.h>
#include <oGUI/window.h>
#include <oGUI/Windows/oWinControlSet.h>
#include <oGUI/Windows/win_gdi_draw.h>
#include <oSystem/filesystem.h>
#include <oSystem/system.h>
#include <oSurface/algo.h>

#include "SystemProperties_xml.h"

using namespace ouro;

static const bool kInteractiveMode = false;

class oSystemProperties
{
public:
	enum CONTROL
	{
		ID_TAB,

		ID_COMPUTER_ICON,
		ID_COMPUTER_FIRST = ID_COMPUTER_ICON,
		ID_COMPUTER_LABEL,
		ID_COMPUTER_DESC_LABEL,
		ID_COMPUTER_DESC_TEXTBOX,
		ID_COMPUTER_DESC_EXAMPLE,
		ID_COMPUTER_NAME_LABEL,
		ID_COMPUTER_NAME_VALUE,
		ID_COMPUTER_WORKGROUP_LABEL,
		ID_COMPUTER_WORKGROUP_VALUE,
		ID_COMPUTER_NETWORK_ID_LABEL,
		ID_COMPUTER_NETWORK_ID_BUTTON,
		ID_COMPUTER_WORKGROUP_RENAME_LABEL,
		ID_COMPUTER_WORKGROUP_RENAME_BUTTON,
		ID_COMPUTER_LAST = ID_COMPUTER_WORKGROUP_RENAME_BUTTON,

		ID_HARDWARE_DEVICE_MANAGER_GROUP,
		ID_HARDWARE_FIRST = ID_HARDWARE_DEVICE_MANAGER_GROUP,
		ID_HARDWARE_DEVICE_MANAGER_ICON,
		ID_HARDWARE_DEVICE_MANAGER_LABEL,
		ID_HARDWARE_DEVICE_MANAGER_BUTTON,
		ID_HARDWARE_DEVICE_INSTALL_GROUP,
		ID_HARDWARE_DEVICE_INSTALL_ICON,
		ID_HARDWARE_DEVICE_INSTALL_LABEL,
		ID_HARDWARE_DEVICE_INSTALL_BUTTON,
		ID_HARDWARE_LAST = ID_HARDWARE_DEVICE_INSTALL_BUTTON,

		ID_ADVANCED_ADMIN_LOGON,
		ID_ADVANCED_FIRST = ID_ADVANCED_ADMIN_LOGON,
		ID_ADVANCED_PERFORMANCE_GROUP,
		ID_ADVANCED_PERFORMANCE_LABEL,
		ID_ADVANCED_PERFORMANCE_SETTINGS,
		ID_ADVANCED_USER_PROFILES_GROUP,
		ID_ADVANCED_USER_PROFILES_LABEL,
		ID_ADVANCED_USER_PROFILES_SETTINGS,
		ID_ADVANCED_STARTUP_GROUP,
		ID_ADVANCED_STARTUP_LABEL,
		ID_ADVANCED_STARTUP_SETTINGS,
		ID_ADVANCED_ENVIRONMENT_VARIABLES,
		ID_ADVANCED_LAST = ID_ADVANCED_ENVIRONMENT_VARIABLES,

		ID_SYSPROT_ICON,
		ID_SYSPROT_FIRST = ID_SYSPROT_ICON,
		ID_SYSPROT_LABEL,
		ID_SYSPROT_RESTORE_TITLE,
		ID_SYSPROT_RESTORE_LABEL,
		ID_SYSPROT_RESTORE_BUTTON,
		ID_SYSPROT_SETTINGS_TITLE,
		ID_SYSPROT_LISTBOX,
		ID_SYSPROT_CONFIG_RESTORE_LABEL,
		ID_SYSPROT_CONFIG_RESTORE_BUTTON,
		ID_SYSPROT_RESTORE_POINT_LABEL,
		ID_SYSPROT_RESTORE_POINT_BUTTON,
		ID_SYSPROT_LAST = ID_SYSPROT_RESTORE_POINT_BUTTON,

		ID_REMOTE_ASSISTANCE_GROUP,
		ID_REMOTE_FIRST = ID_REMOTE_ASSISTANCE_GROUP,
		ID_ALLOW_REMOTE_ASSISTANCE,
		ID_REMOTE_ASSISTANCE_WHAT_HAPPENS,
		ID_REMOTE_ADVANCED,
		ID_REMOTE_DESKTOP_GROUP,
		ID_REMOTE_DESKTOP_CLICK,
		ID_REMOTE_DESKTOP_RADIO1,
		ID_REMOTE_DESKTOP_RADIO2,
		ID_REMOTE_DESKTOP_RADIO3,
		ID_REMOTE_DESKTOP_HELP_ME_CHOOSE,
		ID_REMOTE_DESKTOP_SELECT_USERS,
		ID_REMOTE_LAST = ID_REMOTE_DESKTOP_SELECT_USERS,

		ID_OK,
		ID_CANCEL,
		ID_APPLY,

		NUM_CONTROLS,

		ID_RELOAD_UI,
	};

public:
	oSystemProperties();

	window* GetWindow() { return Window.get(); }
	bool GetRunning() const { return Running; }

	void ShowTab(int _TabIndex)
	{
		oAssert(Window->is_window_thread(), "wrong thread");
		oWinControlSelectSubItem(ControlSet[ID_TAB], _TabIndex);
	}

private:
	oWinControlSet ControlSet;
	std::shared_ptr<window> Window;
	bool Running;

	bool Reload(HWND _hParent, const int2& _ClientSize);

	void Show(int _First, int _Last, bool _Show);
	void Show(int _TabIndex, bool _Show)
	{
		switch (_TabIndex)
		{
			case 0: Show(ID_COMPUTER_FIRST, ID_COMPUTER_LAST, _Show); break;
			case 1: Show(ID_HARDWARE_FIRST, ID_HARDWARE_LAST, _Show); break;
			case 2: Show(ID_ADVANCED_FIRST, ID_ADVANCED_LAST, _Show); break;
			case 3: Show(ID_SYSPROT_FIRST, ID_SYSPROT_LAST, _Show); break;
			case 4: Show(ID_REMOTE_FIRST, ID_REMOTE_LAST, _Show); break;
		}
	}

	void EventHook(const window::basic_event& _Event);
	void ActionHook(const ouro::input_t& _Input);
};

namespace ouro {

bool from_string(oSystemProperties::CONTROL* _pControl, const char* _StrSource)
{
	static const char* sStrings[] = 
	{
		"ID_TAB",

		"ID_COMPUTER_ICON",
		"ID_COMPUTER_LABEL",
		"ID_COMPUTER_DESC_LABEL",
		"ID_COMPUTER_DESC_TEXTBOX",
		"ID_COMPUTER_DESC_EXAMPLE",
		"ID_COMPUTER_NAME_LABEL",
		"ID_COMPUTER_NAME_VALUE",
		"ID_COMPUTER_WORKGROUP_LABEL",
		"ID_COMPUTER_WORKGROUP_VALUE",
		"ID_COMPUTER_NETWORK_ID_LABEL",
		"ID_COMPUTER_NETWORK_ID_BUTTON",
		"ID_COMPUTER_WORKGROUP_RENAME_LABEL",
		"ID_COMPUTER_WORKGROUP_RENAME_BUTTON",

		"ID_HARDWARE_DEVICE_MANAGER_GROUP",
		"ID_HARDWARE_DEVICE_MANAGER_ICON",
		"ID_HARDWARE_DEVICE_MANAGER_LABEL",
		"ID_HARDWARE_DEVICE_MANAGER_BUTTON",
		"ID_HARDWARE_DEVICE_INSTALL_GROUP",
		"ID_HARDWARE_DEVICE_INSTALL_ICON",
		"ID_HARDWARE_DEVICE_INSTALL_LABEL",
		"ID_HARDWARE_DEVICE_INSTALL_BUTTON",

		"ID_ADVANCED_ADMIN_LOGON",
		"ID_ADVANCED_PERFORMANCE_GROUP",
		"ID_ADVANCED_PERFORMANCE_LABEL",
		"ID_ADVANCED_PERFORMANCE_SETTINGS",
		"ID_ADVANCED_USER_PROFILES_GROUP",
		"ID_ADVANCED_USER_PROFILES_LABEL",
		"ID_ADVANCED_USER_PROFILES_SETTINGS",
		"ID_ADVANCED_STARTUP_GROUP",
		"ID_ADVANCED_STARTUP_LABEL",
		"ID_ADVANCED_STARTUP_SETTINGS",
		"ID_ADVANCED_ENVIRONMENT_VARIABLES",

		"ID_SYSPROT_ICON",
		"ID_SYSPROT_LABEL",
		"ID_SYSPROT_RESTORE_TITLE",
		"ID_SYSPROT_RESTORE_LABEL",
		"ID_SYSPROT_RESTORE_BUTTON",
		"ID_SYSPROT_SETTINGS_TITLE",
		"ID_SYSPROT_LISTBOX",
		"ID_SYSPROT_CONFIG_RESTORE_LABEL",
		"ID_SYSPROT_CONFIG_RESTORE_BUTTON",
		"ID_SYSPROT_RESTORE_POINT_LABEL",
		"ID_SYSPROT_RESTORE_POINT_BUTTON",

		"ID_REMOTE_ASSISTANCE_GROUP",
		"ID_ALLOW_REMOTE_ASSISTANCE",
		"ID_REMOTE_ASSISTANCE_WHAT_HAPPENS",
		"ID_REMOTE_ADVANCED",
		"ID_REMOTE_DESKTOP_GROUP",
		"ID_REMOTE_DESKTOP_CLICK",
		"ID_REMOTE_DESKTOP_RADIO1",
		"ID_REMOTE_DESKTOP_RADIO2",
		"ID_REMOTE_DESKTOP_RADIO3",
		"ID_REMOTE_DESKTOP_HELP_ME_CHOOSE",
		"ID_REMOTE_DESKTOP_SELECT_USERS",

		"ID_OK",
		"ID_CANCEL",
		"ID_APPLY",
	};
	match_array(sStrings, oSystemProperties::NUM_CONTROLS);
	for (size_t i = 0; i < countof(sStrings); i++)
	{
		if (!strcmp(_StrSource, sStrings[i]))
		{
			*_pControl = (oSystemProperties::CONTROL)i;
			return true;
		}
	}

	return false;
}

}

oSystemProperties::oSystemProperties()
	: Running(true)
{
	window::init_t i;
	i.title = "TESTWindowSysDialog";
	i.on_event = std::bind(&oSystemProperties::EventHook, this, std::placeholders::_1);
	i.on_input = std::bind(&oSystemProperties::ActionHook, this, std::placeholders::_1);
	i.shape.style = ouro::window_style::dialog;
	i.shape.state = ouro::window_state::restored;
	i.shape.client_size = int2(410,436);
	Window = window::make(i);

	{
		#if oENABLE_ASSERTS
			HWND hWnd = (HWND)Window->native_handle();
			RECT rClient;
			GetClientRect(hWnd, &rClient);
			oAssert(all(oWinRectSize(rClient) == i.shape.client_size), "Client size mismatch");
		#endif

		// Disable anti-aliasing since on Windows ClearType seems to be non-deterministic
		Window->dispatch([&]
		{
			HWND hWnd = (HWND)Window->native_handle();

			ouro::font_info fi = windows::gdi::get_font_info(oWinGetFont(hWnd));
			fi.antialiased = false;
			HFONT hNew = windows::gdi::make_font(fi);
			oWinSetFont(hWnd, hNew);
		});
	}

	if (kInteractiveMode)
	{
		ouro::hotkey HotKeys[] = 
		{
			{ ouro::key::f3, key_modifier::none, ID_RELOAD_UI },
		};

		Window->new_hotkeys(HotKeys);
	}
}

bool oSystemProperties::Reload(HWND _hParent, const int2& _ClientSize)
{
	std::shared_ptr<ouro::xml> XML;

	#if 0
		ouro::path p = ouro::filesystem::dev_path() / "Ouroboros/Source/oGUI/tests/SystemProperties.xml";
		// Load from files for fast iteration, then compile the xml using oFile2cpp
		std::shared_ptr<char> buffer = ouro::filesystem::load(p, ouro::filesystem::load_option::text_read);
		XML = std::make_shared<ouro::xml>(p, buffer.get(), nullptr);
	#else
		XML = std::make_shared<ouro::xml>("cpp:///SystemProperties.xml", (const char*)SystemProperties_xml);
	#endif

	ControlSet.Deinitialize();

	oWinControlSet::IDFROMSTRING FromString = [&](int* _pID, const char* _StrSource)->bool { return ouro::from_string((oSystemProperties::CONTROL*)_pID, _StrSource); };
	ControlSet.Initialize(_hParent, _ClientSize, *XML, FromString);

	if (ControlSet[ID_COMPUTER_NAME_VALUE])
	{
		ouro::mstring Hostname;
		if (kInteractiveMode)
			ouro::system::host_name(Hostname);
		else
			Hostname = "My local hostname";
		oWinControlSetText(ControlSet[ID_COMPUTER_NAME_VALUE], Hostname);
	}

	if (ControlSet[ID_COMPUTER_WORKGROUP_VALUE])
	{
		ouro::mstring WorkgroupName;
		if (kInteractiveMode)
			ouro::system::workgroup_name(WorkgroupName);
		else
			WorkgroupName = "MY_WORKGROUP";
		oWinControlSetText(ControlSet[ID_COMPUTER_WORKGROUP_VALUE], WorkgroupName);
	}

	return true;
}

void oSystemProperties::EventHook(const window::basic_event& _Event)
{
	switch (_Event.type)
	{
		case ouro::event_type::creating:
		{
			oCHECK(Reload((HWND)_Event.window, _Event.as_create().shape.client_size), "");
			break;
		}
	}
}

void oSystemProperties::ActionHook(const ouro::input_t& _Input)
{
	switch (_Input.type)
	{

	case input_type::control:
		switch (_Input.control.status)
		{
			case ouro::control_status::activated:
			{
				switch (_Input.control.id)
				{
					case ID_OK: Running = false; break;
					case ID_CANCEL: Running = false; break;
					case ID_APPLY: oWinEnable((HWND)_Input.control.window, false); break;
					default: break;
				}

				if (_Input.control.id != ID_APPLY)
					oWinEnable((HWND)ControlSet[ID_APPLY], true);
				break;
			}

			case ouro::control_status::selection_changing:
			{
				int index = oWinControlGetSelectedSubItem((HWND)_Input.control.window);
				Show(index, false);
				break;
			}

			case ouro::control_status::selection_changed:
			{
				int index = oWinControlGetSelectedSubItem((HWND)_Input.control.window);
				Show(index, true);
				break;
			}
		}
		break;

		case ouro::input_type::hotkey:
		{
			if (_Input.hotkey.id == ID_RELOAD_UI)
				oCHECK(Reload((HWND)Window->native_handle(), Window->client_size()), "");
			break;
		}
	}
}

void oSystemProperties::Show(int _First, int _Last, bool _Show)
{
	for (int i = _First; i <= _Last; i++)
		oWinControlSetVisible(ControlSet[i], _Show);
}

static void OverwriteVariableColors(ouro::surface::image& _Buffer)
{
	static const int2 VarCoords[] = { int2(25,125), int2(26,125), int2(30,125), int2(31,125), int2(32,125), int2(83,125), int2(84,125), int2(135,125), int2(136,125), int2(137,125), int2(173,125), int2(174,125), int2(175,125), };

	auto subresource_info = ouro::surface::subresourceinfo(_Buffer.info(), 0);
	ouro::surface::lock_guard lock(_Buffer);
	for (const int2& c : VarCoords)
		ouro::surface::put(subresource_info, lock.mapped, c, ouro::color::red);
}

oTEST(oGUI_SysDialog)
{
	if (ouro::system::is_remote_session())
		throw skip_test("Detected remote session: differing text anti-aliasing will cause bad image compares");

	oSystemProperties test;
		
	do
	{
		test.GetWindow()->flush_messages();

	} while (kInteractiveMode && test.GetRunning());

	if (!kInteractiveMode)
	{
		ouro::future<ouro::surface::image> snapshot = test.GetWindow()->snapshot();
		test.GetWindow()->flush_messages();

		ouro::surface::image s = snapshot.get();
		srv.check(s, 0);

		for (int i = 0; i < 5; i++)
		{
			test.ShowTab(i);
			snapshot = test.GetWindow()->snapshot();
			test.GetWindow()->flush_messages();
			s = snapshot.get();
				
			// special-case instance that returns 4 pixels off due to what seems to
			// be indeterminate behavior in win32 rendering (clear type) that cannot
			// be turned off dynamically.
			if (i == 3)
				OverwriteVariableColors(s);

			srv.check(s, i);
		}
	}
}

