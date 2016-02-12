// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oSystem/peripherals.h>
#include <oSystem/windows/win_util.h>

namespace ouro {

static key to_key(WORD vkcode)
{
	static key sKeys[] =
	{
		key::none,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none, // 0x07
		key::backspace,
		key::tab,
		key::none, // 0x0A
		key::none, // 0x0B
		key::none, // VK_CLEAR
		key::enter,
		key::none, // 0x0E
		key::none, // 0x0F
		key::lshift, // VK_SHIFT
		key::lctrl, // VK_CONTROL
		key::lalt, // VK_MENU
		key::pause,
		key::capslock,
		key::none, // VK_KANA
		key::none, //VK_HANGUEL/VK_HANGUL
		key::none, // 0x16
		key::none, // VK_KANA
		key::none, // VK_HANJA/VK_KANJI
		key::none, // 0x1A
		key::esc,
		key::none, // VK_CONVERT
		key::none, // VK_NONCONVERT
		key::none, // VK_ACCEPT
		key::none, // VK_MODECHANGE
		key::space,
		key::pgup,
		key::pgdn,
		key::end,
		key::home,
		key::left,
		key::up,
		key::right,
		key::down,
		key::none, // VK_SELECT
		key::none, // VK_PRINT
		key::none, // VK_EXECUTE
		key::printscreen, // VK_SNAPSHOT
		key::ins,
		key::del,
		key::none, // VK_HELP
		key::_0,
		key::_1,
		key::_2,
		key::_3,
		key::_4,
		key::_5,
		key::_6,
		key::_7,
		key::_8,
		key::_9,
		key::none, // 0x3A
		key::none, // 0x3B
		key::none, // 0x3C
		key::none, // 0x3D
		key::none, // 0x3E
		key::none, // 0x3F
		key::none, // 0x40
		key::a,
		key::b,
		key::c,
		key::d,
		key::e,
		key::f,
		key::g,
		key::h,
		key::i,
		key::j,
		key::k,
		key::l,
		key::m,
		key::n,
		key::o,
		key::p,
		key::q,
		key::r,
		key::s,
		key::t,
		key::u,
		key::v,
		key::w,
		key::x,
		key::y,
		key::z,
		key::lwin,
		key::rwin,
		key::app_context,
		key::none, // 0x5E
		key::sleep,
		key::num0,
		key::num1,
		key::num2,
		key::num3,
		key::num4,
		key::num5,
		key::num6,
		key::num7,
		key::num8,
		key::num9,
		key::nummul,
		key::numadd,
		key::none, // VK_SEPARATOR
		key::numsub,
		key::numdecimal,
		key::numdiv,
		key::f1,
		key::f2,
		key::f3,
		key::f4,
		key::f5,
		key::f6,
		key::f7,
		key::f8,
		key::f9,
		key::f10,
		key::f11,
		key::f12,
		key::f13,
		key::f14,
		key::f15,
		key::f16,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none,
		key::none, // 0x88
		key::none, // 0x89
		key::none, // 0x8A
		key::none, // 0x8B
		key::none, // 0x8C
		key::none, // 0x8D
		key::none, // 0x8E
		key::none, // 0x8F
		key::numlock,
		key::scrolllock,
		key::none, // 0x92
		key::none, // 0x93
		key::none, // 0x94
		key::none, // 0x95
		key::none, // 0x96
		key::none, // 0x97
		key::none, // 0x98
		key::none, // 0x99
		key::none, // 0x9A
		key::none, // 0x9B
		key::none, // 0x9C
		key::none, // 0x9D
		key::none, // 0x9E
		key::none, // 0x9F
		key::lshift,
		key::rshift,
		key::lctrl,
		key::rctrl,
		key::lalt,
		key::ralt,
		key::back,
		key::forward,
		key::refresh,
		key::stop,
		key::search,
		key::favs,
		key::home,
		key::mute,
		key::voldn,
		key::volup,
		key::next_track,
		key::prev_track,
		key::stop_track,
		key::play_pause_track,
		key::mail,
		key::media,
		key::app1,
		key::app2,
		key::none, // 0xB8
		key::none, // 0xB9
		key::semicolon,
		key::equal_,
		key::comma,
		key::dash,
		key::period,
		key::slash,
		key::backtick,

		key::none, // 0xC1
		key::none, // 0xC2
		key::none, // 0xC3
		key::none, // 0xC4
		key::none, // 0xC5
		key::none, // 0xC6
		key::none, // 0xC7
		key::none, // 0xC8
		key::none, // 0xC9
		key::none, // 0xCA
		key::none, // 0xCB
		key::none, // 0xCC
		key::none, // 0xCD
		key::none, // 0xCE
		key::none, // 0xCF

		key::none, // 0xD0
		key::none, // 0xD1
		key::none, // 0xD2
		key::none, // 0xD3
		key::none, // 0xD4
		key::none, // 0xD5
		key::none, // 0xD6
		key::none, // 0xD7
		key::none, // 0xD8
		key::none, // 0xD9
		key::none, // 0xDA
		key::lbracket,
		key::backslash,
		key::rbracket,
		key::apostrophe,
		key::none, // VK_OEM_8
		key::none, // 0xE0
		key::none, // 0xE1
		key::none, // VK_OEM_102
		key::none, // 0xE3
		key::none, // 0xE4
		key::none, // VK_PROCESSKEY
		key::none, // 0xE6
		key::none, // VK_PACKET
		key::none, // 0xE8
		key::none, // 0xE9
		key::none, // 0xEA
		key::none, // 0xEB
		key::none, // 0xEC
		key::none, // 0xED
		key::none, // 0xEE
		key::none, // 0xEF
		key::none, // 0xF0
		key::none, // 0xF1
		key::none, // 0xF2
		key::none, // 0xF3
		key::none, // 0xF4
		key::none, // 0xF5
		key::none, // VK_ATTN
		key::none, // VK_CRSEL
		key::none, // VK_EXSEL
		key::none, // VK_EREOF
		key::none, // VK_PLAY
		key::none, // VK_ZOOM
		key::none, // VK_NONAME
		key::none, // VK_PA1
		key::none, // VK_OEM_CLEAR
		key::none, // 0xFF
	};
	match_array(sKeys, 256);
	
	if (vkcode >= VK_XBUTTON2 && sKeys[vkcode] == key::none)
		oTRACE("No mapping for vkcode = %x", vkcode);
	
	return sKeys[vkcode];
}

static WORD from_key(const key& key)
{
	static const uint8_t sVKCodes[] = 
	{
		0,

		// modifier keys
		VK_LCONTROL, VK_RCONTROL,
		VK_LMENU, VK_RMENU,
		VK_LSHIFT, VK_RSHIFT,
		VK_LWIN, VK_RWIN,
		0, VK_APPS,

		// toggle keys
		VK_CAPITAL,
		VK_SCROLL,
		VK_NUMLOCK,

		// typing keys
		VK_SPACE, VK_OEM_3, VK_OEM_MINUS, VK_OEM_PLUS, VK_OEM_4, VK_OEM_5, VK_OEM_6, VK_OEM_1, VK_OEM_7, VK_OEM_COMMA, VK_OEM_PERIOD, VK_OEM_2,
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',

		// numpad keys
		VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9,
		VK_MULTIPLY, VK_ADD, VK_SUBTRACT, VK_DECIMAL, VK_DIVIDE,

		// typing control keys
		VK_ESCAPE,
		VK_BACK,
		VK_TAB,
		VK_RETURN,
		VK_INSERT, VK_DELETE,
		VK_HOME, VK_END,
		VK_PRIOR, VK_NEXT,

		// system control keys
		VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, VK_F13, VK_F14, VK_F15, VK_F16, 
		VK_PAUSE,
		VK_SLEEP,
		VK_SNAPSHOT,

		// directional keys
		VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN,

		// browser keys
		VK_LAUNCH_MAIL,
		VK_BROWSER_BACK, 
		VK_BROWSER_FORWARD, 
		VK_BROWSER_REFRESH, 
		VK_BROWSER_STOP, 
		VK_BROWSER_SEARCH, 
		VK_BROWSER_FAVORITES,

		// media keys
		VK_LAUNCH_MEDIA_SELECT, 
		VK_VOLUME_MUTE, 
		VK_VOLUME_UP, 
		VK_VOLUME_DOWN, 
		VK_MEDIA_PREV_TRACK, 
		VK_MEDIA_NEXT_TRACK, 
		VK_MEDIA_STOP, 
		VK_MEDIA_PLAY_PAUSE,

		// misc keys
		VK_LAUNCH_APP1, VK_LAUNCH_APP2,
	};
	match_array_e(sVKCodes, key);
	return sVKCodes[(uint8_t)key];
}

// hwnd, xy required for mouse button presses
static INPUT to_input(WORD vkcode, bool down, HWND hwnd = nullptr, int16_t x = 0, int16_t y = 0)
{
	INPUT i;

	switch (vkcode)
	{
		case VK_XBUTTON1: case VK_XBUTTON2: throw std::invalid_argument("extended buttons 1 & 2 not supported");
		case VK_LBUTTON: case VK_RBUTTON: case VK_MBUTTON:
		{
			static const uint16_t sUps[]   =  { MOUSEEVENTF_LEFTUP,   MOUSEEVENTF_RIGHTUP,   MOUSEEVENTF_MIDDLEUP   };
			static const uint16_t sDowns[] =  { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_MIDDLEDOWN };
			const DWORD flags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | ((down ? sDowns : sUps)[vkcode >> 1]);

			WINDOWPLACEMENT wp;
			GetWindowPlacement(hwnd, &wp);

			i.type = INPUT_MOUSE;
			i.mi.dx = (65535 * ((long)x + wp.rcNormalPosition.left)) / GetSystemMetrics(SM_CXSCREEN);
			i.mi.dy = (65535 * ((long)y + wp.rcNormalPosition.top)) / GetSystemMetrics(SM_CYSCREEN);
			i.mi.mouseData = 0;
			i.mi.dwFlags = flags;
			i.mi.time = 0;
			i.mi.dwExtraInfo = GetMessageExtraInfo();
			break;
		}
	
		default:
		{
			i.type = INPUT_KEYBOARD;
			i.ki.wVk = vkcode;
			i.ki.time = 0;
			i.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
			i.ki.dwExtraInfo = GetMessageExtraInfo();
			break;
		}
	}

	return i;
}

keyboard_t::keyboard_t()
{
	initialize();
}

keyboard_t::~keyboard_t()
{
	deinitialize();
}

void keyboard_t::initialize()
{
	memset(this, 0, sizeof(*this));
}

void keyboard_t::deinitialize()
{
	memset(this, 0, sizeof(*this));
}
	
void keyboard_t::update()
{
	for (int i = 0; i < countof(msg_dn); i++)
	{
		const uint64_t pre_dn = cur_dn[i];

		cur_dn[i] |= msg_dn[i];
		cur_dn[i] &= ~msg_up[i];
		msg_dn[i] = msg_up[i] = 0;

		const uint64_t changed = pre_dn ^ cur_dn[i];
		cur_pr[i] = changed & cur_dn[i];
		cur_re[i] = changed & pre_dn;
	}
}

void keyboard_t::reset()
{
	memset(this, 0, sizeof(*this));
}

void keyboard_t::trigger(const key_event& e)
{
	uint64_t* dst = e.down ? msg_dn : msg_up;
	int index;
	uint64_t mask;
	index_and_mask(e.key, &index, &mask);
	dst[index] |= mask;
}

void keyboard_t::send(void* hwnd, const key& key, bool down, int16_t x, int16_t y)
{
	const DWORD cur_tid = GetCurrentThreadId();
	const DWORD tid = GetWindowThreadProcessId((HWND)hwnd, nullptr);
	AttachThreadInput(cur_tid, tid, true);
	INPUT i = to_input(from_key(key), down);
	SendInput(1, &i, sizeof(i));
	AttachThreadInput(cur_tid, asdword(tid), false);
}

void keyboard_t::send(void* hwnd, const char* msg)
{
	const size_t len = strlen(msg);
	const HKL hkl = GetKeyboardLayout(0);
	const bool capslock_down = GetKeyState(VK_CAPITAL) != 0;

	WORD* vkeys = (WORD*)alloca(sizeof(WORD) * len);
	for (size_t i = 0; i < len; i++)
		vkeys[i] = VkKeyScanEx(msg[i], hkl);

	INPUT* inputs = (INPUT*)alloca(len * 2 + 4); // 2x for up and down + 4 for caps lock
	INPUT* inp = inputs;
	
	if (capslock_down)
	{
		*inp++ = to_input(VK_CAPITAL, true);
		*inp++ = to_input(VK_CAPITAL, false);
	}

	for (size_t i = 0; i < len; i++)
	{
		const WORD vkcode = vkeys[i];
		const bool apply_shift = (0x0100 & vkcode) != 0;

		if (apply_shift)
			*inp++ = to_input(VK_SHIFT, true);

		*inp++ = to_input(vkcode, true);
		*inp++ = to_input(vkcode, false);

		if (apply_shift)
			*inp++ = to_input(VK_SHIFT, false);
	}

	if (capslock_down)
	{
		*inp++ = to_input(VK_CAPITAL, true);
		*inp++ = to_input(VK_CAPITAL, false);
	}

	const DWORD cur_tid = GetCurrentThreadId();
	const DWORD tid = GetWindowThreadProcessId((HWND)hwnd, nullptr);
	AttachThreadInput(cur_tid, tid, true);

	const UINT nkeys = UINT(inp - inputs);
	SendInput(nkeys, inputs, sizeof(INPUT));

	AttachThreadInput(cur_tid, tid, false);
}

static void hotkeys_to_accels(size_t num_entries, const hotkey* hotkeys, void* accels)
{
	for (size_t i = 0; i < num_entries; i++)
	{
		ACCEL& a = ((ACCEL*)accels)[i];
		const hotkey& h = hotkeys[i];
		a.fVirt = FVIRTKEY;
		
		if ((uint8_t)h.modifier & (uint8_t)key_modifier::ctrl) a.fVirt |= FCONTROL;
		if ((uint8_t)h.modifier & (uint8_t)key_modifier::alt) a.fVirt |= FALT;
		if ((uint8_t)h.modifier & (uint8_t)key_modifier::shift) a.fVirt |= FSHIFT;
		
		a.key = from_key(h.key) & 0xffff;
		a.cmd = h.id;
	}
}

static void accels_to_hotkeys(size_t num_entries, const void* accels, hotkey* hotkeys)
{
	for (size_t i = 0; i < num_entries; i++)
	{
		const ACCEL& a = ((const ACCEL*)accels)[i];
		oASSERT(a.fVirt & FVIRTKEY, "");

		uint8_t modifiers = 0;
		if (a.fVirt & FCONTROL) modifiers |= (uint8_t)key_modifier::ctrl;
		if (a.fVirt & FALT) modifiers |= (uint8_t)key_modifier::alt;
		if (a.fVirt & FSHIFT) modifiers |= (uint8_t)key_modifier::shift;

		hotkey& h = hotkeys[i];
		h.key = to_key(a.key);
		h.id = a.cmd;
		h.modifier = (key_modifier)modifiers;
		h.id = a.cmd;
	}
}

keyboard_t::hotkeyset_t keyboard_t::new_hotkeyset(const hotkey* hotkeys, size_t num_hotkeys)
{
	size_t bytes = sizeof(ACCEL) * num_hotkeys;
	oASSERT(bytes <= 64 * 1024, "need a better alloc");
	LPACCEL accels = (LPACCEL)alloca(bytes);
	hotkeys_to_accels(num_hotkeys, hotkeys, accels);
	HACCEL hAccel = CreateAcceleratorTable(accels, (uint32_t)num_hotkeys);
	return (hotkeyset_t)hAccel;
}

void keyboard_t::del_hotkeyset(hotkeyset_t hotkeyset)
{
	if (hotkeyset)
		DestroyAcceleratorTable((HACCEL)hotkeyset);
}

uint32_t keyboard_t::decode_hotkeyset(hotkeyset_t hotkeyset, hotkey* hotkeys, size_t max_hotkeys)
{
	uint32_t num_hotkeys = 0;

	if (hotkeyset)
	{
		num_hotkeys = (uint32_t)__min((int)max_hotkeys, CopyAcceleratorTable((HACCEL)hotkeyset, nullptr, 0));
		size_t bytes = sizeof(ACCEL) * num_hotkeys;
		LPACCEL accels = (LPACCEL)alloca(bytes);
		CopyAcceleratorTable((HACCEL)hotkeyset, accels, num_hotkeys);
		accels_to_hotkeys(num_hotkeys, accels, hotkeys);
	}

	return num_hotkeys;
}

bool handle_hotkey(void* hwnd, uint32_t msg, uintptr_t wparam, uintptr_t lparam, hotkey_event* out)
{
	if (msg == WM_COMMAND && !lparam && HIWORD(wparam) == 1)
	{
		out->window = hwnd;
		out->id = LOWORD(wparam);
		return true;
	}
	return false;
}

// Expands control keys to their proper left/right identifiers as an ouro::key
key translate_vkcode(WORD vkcode, uintptr_t lparam)
{
	// adjust wparam to be more specific
	uint32_t scancode = (lparam & 0x00ff0000) >> 16;
	bool extended = !!(lparam & 0x01000000);

	switch (vkcode)
	{
		case VK_SHIFT:   vkcode = (WORD)MapVirtualKey(scancode, MAPVK_VSC_TO_VK_EX); break;
		case VK_CONTROL: vkcode = extended ? VK_RCONTROL : VK_LCONTROL;        break;
		case VK_MENU:    vkcode = extended ? VK_RMENU    : VK_LMENU;           break;
		default: break;
	}

	return to_key(vkcode);
}

int handle_keyboard(void* hwnd, uint32_t msg, uintptr_t wparam, uintptr_t lparam, key_event* out)
{
	bool dn = true;
	bool key = false;
	int handled = 1;

	switch (msg)
	{
		case WM_KEYUP:    dn = false; case WM_KEYDOWN: key = true; break;
		case WM_SYSKEYUP: dn = false; 
		case WM_SYSKEYDOWN:
		{
			key = true;
			bool is_alt (wparam == VK_MENU || wparam == VK_LMENU || wparam == VK_RMENU);
			handled = is_alt ? 1 : -1;
			break;
		}

		default: break;
	}

	if (key)
	{
		key_event e;
		e.key = translate_vkcode((WORD)wparam, lparam);
		e.down = dn;

		if (out)
			*out = e;

		return handled;
	}

	return 0;
}

}
