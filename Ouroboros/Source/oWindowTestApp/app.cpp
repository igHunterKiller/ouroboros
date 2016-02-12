// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "app.h"

#include <oCore/countof.h>
#include <oSystem/reporting.h>
#include <oGUI/msgbox_reporting.h>

namespace ouro {

void app::initialize(const init_t& i)
{
	running = false;

	try
	{
		reporting::set_prompter(prompt_msgbox);

		if (i.num_watchdirs)
		{
			filesystem::monitor::info inf;
			inf.accessibility_poll_rate_ms = 2000;
			inf.accessibility_timeout_ms = 5000;
			dirwatcher = filesystem::monitor::make(inf, std::bind(&app::internal_on_dir_event, this, std::placeholders::_1, std::placeholders::_2));

			for (uint32_t ii = 0; ii < i.num_watchdirs; ii++)
				dirwatcher->watch(i.watchdirs[ii], 64 * 1024, true);
		}

		for (uint8_t ii = 0; ii < countof(pads); ii++)
			pads[ii].initialize(ii);
		keyboard.initialize();
		mouse.initialize();

		window = window::make(i.window_init);
	}

	catch (std::exception&)
	{
		deinitialize();
		std::rethrow_exception(std::current_exception());
	}
}

void app::deinitialize()
{
	window = nullptr;
	mouse.deinitialize();
	keyboard.deinitialize();
	for (auto& c : pads)
		c.deinitialize();
}

void app::internal_on_dir_event(filesystem::file_event::value e, const path_t& p)
{
	on_dir_event(e, p);
}

void app::run()
{
	running = true;
	while (running)
	{
		// Handle all inputs and events
		{
			window->flush_messages();
			for (auto& c : pads)
				c.update();
			keyboard.update();
			mouse.update();
		}
	}
}

}
