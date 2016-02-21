// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

#include <oGUI/window.h>
#include <oSystem/peripherals.h>
#include <oSystem/filesystem_monitor.h>

namespace ouro {

class app
{
public:
	struct init_t
	{
		init_t()
			: msgbox_error_prompt(true)
		{}

		bool msgbox_error_prompt;

		const char** watchdirs;
		uint32_t num_watchdirs;

		window::init_t window_init;
	};

	app() {}
	app(const init_t& i) { initialize(i); }
	~app() { deinitialize(); }

	void initialize(const init_t& i);
	void deinitialize();

	void run();
	void stop() { running = false; }

	
	virtual void on_dir_event(filesystem::file_event e, const path_t& p) {}

protected:
	static void internal_on_dir_event(filesystem::file_event e, const path_t& p, void* user) { ((app*)user)->on_dir_event(e, p); }

	std::shared_ptr<window> window;
	pad_t pads[pad_t::max_num_pads];
	keyboard_t keyboard;
	mouse_t mouse;
	std::shared_ptr<filesystem::monitor> dirwatcher;

	bool running;
};

}
