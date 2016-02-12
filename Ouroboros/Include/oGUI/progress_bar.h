// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A simple progress bar for very simple applications. NOTE: This is for the 90%
// case. Dialogs are fairly easy to construct and should be tailored to the 
// task,so if there's a complex case such as wanting to see individual task v. 
// overall progress, or seeing progress of each thread, a different dialog box 
// should be created. In the meantime, ALL components of a major task should add 
// up to the single completion of the progress bar. I.e. don't have Step1 go to 
// 100% and then Step2 resets and goes to 100%, just update the task and have 
// each add up to 50% of the overall progress. See GUI suggestions on the net. 
// There's a good starting point at:
// http://msdn.microsoft.com/en-us/library/aa511486.aspx

#pragma once
#include <oGUI/window.h>

namespace ouro {

class progress_bar : public basic_window
{
public:
	static std::shared_ptr<progress_bar> make(const char* _Title
		, ouro::icon_handle _hIcon, const std::function<void()>& _OnStop);

	virtual void stop_button(bool _Show) = 0;
	virtual bool stop_button() const = 0;

	virtual void stopped(bool _Stopped) = 0;
	virtual bool stopped() const = 0;

	virtual void set_textv(const char* _Format, va_list _Args) = 0;
	virtual char* get_text(char* _StrDestination, size_t _SizeofStrDestination) const = 0;

	inline void set_text(const char* _Format, ...) { va_list args; va_start(args, _Format); set_textv(_Format, args); va_end(args); }
	template<size_t size> char* get_text(char (&_StrDestination)[size]) const { return get_text(_StrDestination, size); }
	template<size_t capacity> char* get_text(fixed_string<char, capacity>& _StrDestination) const { return get_text(_StrDestination, _StrDestination.capacity()); }

	virtual void set_subtextv(const char* _Format, va_list _Args) = 0;
	virtual char* get_subtext(char* _StrDestination, size_t _SizeofStrDestination) const = 0;

	inline void set_subtext(const char* _Format, ...) { va_list args; va_start(args, _Format); set_subtextv(_Format, args); va_end(args); }
	template<size_t size> char* get_subtext(char (&_StrDestination)[size]) const { return get_subtext(_StrDestination, size); }
	template<size_t capacity> char* get_subtext(fixed_string<char, capacity>& _StrDestination) const { return get_subtext(_StrDestination, _StrDestination.capacity()); }

	// Sets the percentage complete. This value is internally clamped to [0,100], 
	// unless set to a negative value in which case an unknown progress display is
	// shown (marquee).
	virtual void set_percentage(int _Percentage) = 0;
	virtual int percentage() const = 0; 

	// Adds the amount (clamps total [0,100]). This way several threads can pre-
	// calculate their individual contribution and asynchronously add it to the
	// progress. If this number goes negative then an unknown progress display is
	// shown (marquee).
	virtual void add_percentage(int _Percentage) = 0;
};

}
