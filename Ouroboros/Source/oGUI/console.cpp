// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/console.h>
#include <oGUI/Windows/oWinWindowing.h>
//#include <oGUI/Windows/oWinRect.h>
#include <oSystem/windows/win_error.h>
#include <array>

using namespace std;

namespace ouro { namespace console {

#define FOREGROUND_GRAY (FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE)
#define BACKGROUND_GRAY (BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE)
#define FOREGROUND_MASK (FOREGROUND_INTENSITY|FOREGROUND_GRAY)
#define BACKGROUND_MASK (BACKGROUND_INTENSITY|BACKGROUND_GRAY)

// returns prior attributes
static WORD set_console_color(HANDLE hstream, uint8_t fg, uint8_t bg)
{
	#define RED__    FOREGROUND_RED       | BACKGROUND_RED
	#define GREEN__  FOREGROUND_GREEN     | BACKGROUND_GREEN
	#define BLUE__   FOREGROUND_BLUE      | BACKGROUND_BLUE
	#define BRIGHT__ FOREGROUND_INTENSITY | BACKGROUND_INTENSITY

	static const WORD sConsoleColorWords[] = { 0, BLUE__, GREEN__, BLUE__|GREEN__, RED__, RED__|BLUE__, RED__|GREEN__, RED__|GREEN__|BLUE__, BRIGHT__, BRIGHT__|BLUE__, BRIGHT__|GREEN__, BRIGHT__|BLUE__|GREEN__, BRIGHT__|RED__, BRIGHT__|RED__|BLUE__, BRIGHT__|RED__|GREEN__, BRIGHT__|RED__|GREEN__|BLUE__ };

	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(hstream, &inf);
	WORD wOriginalAttributes = inf.wAttributes;
	WORD wAttributes = inf.wAttributes & ~(FOREGROUND_MASK|BACKGROUND_MASK);

	wAttributes |= (fg | (bg << 4)) & (FOREGROUND_MASK|BACKGROUND_MASK);
	
	SetConsoleTextAttribute(hstream, wAttributes);
	return wOriginalAttributes;
}

void set_title(const char* title)
{
	oVB(SetConsoleTitleA(title));
}

char* get_title(char* dst, size_t dst_size)
{
	return GetConsoleTitleA(dst, static_cast<DWORD>(dst_size)) ? dst : nullptr;
}

void icon(ouro::icon_handle icon)
{
	oWinSetIconAsync(GetConsoleWindow(), (HICON)icon);
}

ouro::icon_handle icon()
{
	return (ouro::icon_handle)oWinGetIcon(GetConsoleWindow());
}

void focus(bool focus)
{
	oWinSetFocus(GetConsoleWindow(), focus);
}

bool has_focus()
{
	return oWinHasFocus(GetConsoleWindow());
}

int2 size_pixels()
{
	RECT r;
	GetWindowRect(GetConsoleWindow(), &r);
	return int2(r.right - r.left, r.bottom - r.top);
}

int2 size_characters()
{
	CONSOLE_SCREEN_BUFFER_INFO inf;
	oVB(GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &inf));
	return int2(inf.dwSize.X, inf.dwSize.Y);
}

void cursor_position(const int2& position)
{
	COORD c = { static_cast<short>(position.x), static_cast<short>(position.y) };
	if (c.X == use_default || c.Y == use_default)
	{
		CONSOLE_SCREEN_BUFFER_INFO inf;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &inf);
		if (c.X == use_default) c.X = inf.dwCursorPosition.X;
		if (c.Y == use_default) c.Y = inf.dwCursorPosition.Y;
	}

	oVB(SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c));
}

int2 cursor_position()
{
	CONSOLE_SCREEN_BUFFER_INFO inf;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &inf);
	return int2(inf.dwCursorPosition.X, inf.dwCursorPosition.Y);
}

void clear()
{
	::system("cls");
}
















class context
{
public:
	context()
		: ctrl_handler_set_(false)
		, hlog_(nullptr)
	{
		handlers_.fill(nullptr);
		users_.fill(nullptr);
	}

	~context()
	{
		if (hlog_)
			filesystem::close(hlog_);
	}

	static context& singleton();

	ouro::window_handle native_handle() const { return (ouro::window_handle)GetConsoleWindow(); }
  info_t get_info() const;
  void set_info(const info_t& info);

	void set_log(const path_t& path);
	path_t get_log() const;

  void set_handler(signal_t signal, signal_handler_fn signal_handler, void* user);
  int vfprintf(FILE* stream, uint8_t fg, uint8_t bg, const char* _Format, va_list _Args);
private:
	typedef recursive_mutex mutex_t;
	typedef lock_guard<mutex_t> lock_t;
	mutable mutex_t mutex_;
	std::array<signal_handler_fn, 5> handlers_;
	std::array<void*, 5> users_;
	bool ctrl_handler_set_;

	path_t log_path_;
	filesystem::file_handle hlog_;
	
	BOOL ctrl_handler(DWORD fdwCtrlType);
	static BOOL CALLBACK static_ctrl_handler(DWORD fdwCtrlType) { return singleton().ctrl_handler(fdwCtrlType); }
};

oDEFINE_PROCESS_SINGLETON("ouro::gui::console", context);

console::info_t context::get_info() const
{
	lock_t lock(mutex_);
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	oAssert(GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE, "");

	if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &sbi))
		oCheck(GetLastError() != ERROR_INVALID_HANDLE || !this_process::is_child(), std::errc::permission_denied, "Failed to access console because this is a child process.");

	info_t i;
	i.window_position = int2(sbi.srWindow.Left, sbi.srWindow.Top);
	i.window_size = int2(sbi.srWindow.Right - sbi.srWindow.Left, sbi.srWindow.Bottom - sbi.srWindow.Top);
	i.buffer_size = int2(sbi.dwSize.X, sbi.dwSize.Y);
	i.foreground = uint8_t(sbi.wAttributes);
	i.background = uint8_t(sbi.wAttributes >> 4);
	i.show = !!IsWindowVisible(GetConsoleWindow());
	return i;
}

void context::set_info(const info_t& info)
{
	lock_t lock(mutex_);

	info_t i = get_info();
	#define DEF(x) if (info.x != use_default) i.x = info.x
	DEF(buffer_size.x);
	DEF(buffer_size.y);
	DEF(window_position.x);
	DEF(window_position.y);
	DEF(window_size.x);
	DEF(window_size.y);
	i.foreground = info.foreground;
	i.background = info.background;
	i.show = info.show;
	#undef DEF

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	COORD bufferDimension = { static_cast<SHORT>(i.buffer_size.x), static_cast<SHORT>(i.buffer_size.y) };
	if (!SetConsoleScreenBufferSize(hConsole, bufferDimension))
		oCheck(GetLastError() != ERROR_INVALID_HANDLE || !ouro::this_process::is_child(), std::errc::permission_denied, "Failed to access console because this is a child process.");

	SMALL_RECT r;
	r.Left = 0;
	r.Top = 0;
	r.Right = static_cast<SHORT>(r.Left + i.window_size.x);
	r.Bottom = static_cast<SHORT>(r.Top + i.window_size.y);

	// Clamp to max size.
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	oVB(GetConsoleScreenBufferInfo(hConsole, &sbi));
	if (r.Right >= sbi.dwMaximumWindowSize.X)
	{
		r.Right = sbi.dwMaximumWindowSize.X - 1;
		if (bufferDimension.X <= i.window_size.x)
			oTrace("Clamping console width (%d) to system max of %d due to a specified buffer width (%d) larger than screen width", i.buffer_size.x, r.Right, bufferDimension.X);
		else
			oTrace("Clamping console width (%d) to system max of %d", i.buffer_size.x, r.Right);
	}

	if (r.Bottom >= sbi.dwMaximumWindowSize.Y)
	{
		r.Bottom = sbi.dwMaximumWindowSize.Y - 4; // take a bit more off for the taskbar
		if (bufferDimension.Y <= i.window_size.y)
			oTrace("Clamping console height (%d) to system max of %d due to a specified buffer width (%d) larger than screen width", i.buffer_size.y, r.Bottom, bufferDimension.Y);
		else
			oTrace("Clamping console height (%d) to system max of %d", i.buffer_size.y, r.Bottom);
	}

	oVB(SetConsoleWindowInfo(hConsole, TRUE, &r));
	UINT show = i.show ? SWP_SHOWWINDOW : SWP_HIDEWINDOW;
	oVB(SetWindowPos(GetConsoleWindow(), HWND_TOP, i.window_position.x, i.window_position.y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|show));
	set_console_color(hConsole, info.foreground, info.background);
}

void context::set_log(const path_t& path)
{
	if (hlog_)
	{
		filesystem::close(hlog_);
		hlog_ = nullptr;
	}

	log_path_ = path;
	if (!log_path_.empty())
	{
		filesystem::create_directories(path.parent_path());
		hlog_ = filesystem::open(log_path_, filesystem::open_option::text_append);
	}
}

path_t context::get_log() const
{
	lock_t lock(mutex_);
	return log_path_;
}

int context::vfprintf(FILE* stream, uint8_t fg, uint8_t bg, const char* _Format, va_list _Args)
{
	HANDLE hConsole = 0;
	WORD wOriginalAttributes = 0;

	lock_t lock(mutex_);

	if (stream == stdout || stream == stderr)
	{
		hConsole = GetStdHandle(stream == stderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
		wOriginalAttributes = set_console_color(hConsole, fg, bg);
	}

	xlstring msg;
	vsnprintf(msg, _Format, _Args);

	// Always print any message to stream
	int n = ::fprintf(stream, msg);

	// And to log file
	if (hlog_)
		filesystem::write(hlog_, msg, strlen(msg), true);

	if (hConsole)
		SetConsoleTextAttribute(hConsole, wOriginalAttributes);

	return n;
}

BOOL context::ctrl_handler(DWORD fdwCtrlType)
{
	static const signal_t s_remap[] = { signal_t::ctrl_c, signal_t::ctrl_break, signal_t::close, signal_t::close,signal_t::close, signal_t::logoff, signal_t::shutdown };
	signal_t sig = s_remap[fdwCtrlType];
	auto fn = handlers_[(int)sig];
	void* user = users_[(int)sig];
	if (fn) fn(user);
	return FALSE;
}

void context::set_handler(signal_t signal, signal_handler_fn signal_handler, void* user)
{
	lock_t lock(mutex_);
	handlers_[(int)signal] = signal_handler;
	users_[(int)signal] = user;
	if (!ctrl_handler_set_)
	{
		oVB(SetConsoleCtrlHandler(static_ctrl_handler, TRUE));
		ctrl_handler_set_ = true;
	}
}

ouro::window_handle native_handle()
{
	return context::singleton().native_handle();
}
  
info_t get_info()
{
	return context::singleton().get_info();
}

void set_info(const info_t& info)
{
	context::singleton().set_info(info);
}

void set_log(const path_t& path)
{
	context::singleton().set_log(path);
}

path_t get_log()
{
	return context::singleton().get_log();
}

void set_handler(signal_t signal, signal_handler_fn signal_handler, void* user)
{
	context::singleton().set_handler(signal, signal_handler, user);
}

int vfprintf(FILE* stream, uint8_t fg, uint8_t bg, const char* _Format, va_list _Args)
{
	return context::singleton().vfprintf(stream, fg, bg, _Format, _Args);
}

}}
