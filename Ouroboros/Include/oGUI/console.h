// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// interface for working with a console in a GUI environment

#pragma once
#include <oCore/color.h>
#include <oString/path.h>
#include <oGUI/oGUI.h>
#include <cstdarg>
#include <functional>

namespace ouro { namespace console {

static const int use_default = 0x8000;

enum class signal_t
{
  ctrl_c,
  ctrl_break,
  close,
  logoff,
  shutdown,
};

struct info_t
{
	info_t()
		: window_position(use_default, use_default)
		, window_size(use_default, use_default)
		, buffer_size(use_default, use_default)
		, foreground(console_color::black)
		, background(console_color::white)
		, show(true)
	{}

  int2 window_position;
  int2 window_size;
  int2 buffer_size;
  uint8_t foreground; // a console_color
  uint8_t background; // a console_color
	bool show;
};
  
ouro::window_handle native_handle();
  
info_t get_info();
void set_info(const info_t& info);
  
inline int2 position() { info_t i = get_info(); return i.window_position; }
inline void position(const int2& position) { info_t i = get_info(); i.window_position = position; set_info(i); }
  
inline int2 size() { info_t i = get_info(); return i.window_size; }
inline void size(const int2& size) { info_t i = get_info(); i.window_size = size; set_info(i); }
        
void set_title(const char* title);
char* get_title(char* dst, size_t dst_size);
template<size_t size> char* get_title(char (&dst)[size]) { return get_title(dst, size); }
template<size_t capacity> char* get_title(fixed_string<char, capacity>& dst) { return get_title(dst, dst.capacity()); }

// Specifying empty string disables logging
void set_log(const path_t& path);
path_t get_log();

void icon(ouro::icon_handle icon);
ouro::icon_handle icon();
  
void focus(bool focus = true);
bool has_focus();
  
int2 size_pixels();
int2 size_characters();
  
void cursor_position(const int2& position);
int2 cursor_position();
  
// The handler should return true to short-circuit any default behavior, or 
// false to allow default behavior to continue.
typedef bool (*signal_handler_fn)(void* user);
void set_handler(signal_t signal, signal_handler_fn signal_handler, void* user);
  
void clear();
 
// use console_color's for fg and bg
       int vfprintf(FILE* stream, uint8_t fg, uint8_t bg, const char* format, va_list args);
inline int vfprintf(FILE* stream, uint8_t fg,             const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stream, fg, 0,  format, args); va_end(args); return n; }
inline int vfprintf(FILE* stream,                         const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stream, 0,  0,  format, args); va_end(args); return n; }
inline int  fprintf(FILE* stream, uint8_t fg, uint8_t bg, const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stream, fg, bg, format, args); va_end(args); return n; }
inline int  fprintf(FILE* stream, uint8_t fg,             const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stream, fg, 0,  format, args); va_end(args); return n; }
inline int  fprintf(FILE* stream,                         const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stream, 0,  0,  format, args); va_end(args); return n; }
inline int  vprintf(              uint8_t fg, uint8_t bg, const char* format, va_list args) {                                       int n = vfprintf(stdout, fg, bg, format, args);               return n; }
inline int  vprintf(              uint8_t fg,             const char* format, va_list args) {                                       int n = vfprintf(stdout, fg, 0,  format, args);               return n; }
inline int  vprintf(                                      const char* format, va_list args) {                                       int n = vfprintf(stdout, 0,  0,  format, args);               return n; }
inline int   printf(              uint8_t fg, uint8_t bg, const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stdout, fg, bg, format, args); va_end(args); return n; }
inline int   printf(              uint8_t fg,             const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stdout, fg, 0,  format, args); va_end(args); return n; }
inline int   printf(                                      const char* format, ...)          { va_list args; va_start(args, format); int n = vfprintf(stdout, 0,  0,  format, args); va_end(args); return n; }

}}
