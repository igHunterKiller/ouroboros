// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oSystem/process_killer.h>
#include <oSystem/module.h>
#include <oGUI/msgbox.h> // @tony: BAD DEPENDENCY: move this object to a unit_test module or something.

namespace ouro {

void process_killer::add(const entry_t& entry)
{
	if (entry.process_name && entry.process_name[0] != '\0')
	{
		internal_entry_t e;
		e.process_name = entry.process_name;
		e.type = entry.type;
		e.pid = process::get_id(e.process_name);
		entries.push_back(std::move(e));
	}
}

bool process_killer::kill(const char* msgbox_title, bool prompt_user)
{
	char msg[4096];
	char* m = msg;
	int start = snprintf(msg, "The following active processes will interfere with tests either by using resources during benchmarks or by altering image compares:\n\n");

	std::vector<process::id> kill, prompt, inform;
	kill.reserve(50);
	prompt.reserve(50);
	inform.reserve(50);

	for (const auto& e : entries)
	{
		if (!e.pid)
			continue;

		switch (e.type)
		{
			case auto_kill:      kill.push_back(e.pid);   break;
			case inform_to_kill: inform.push_back(e.pid); break;
			case prompt_to_kill:
			{
				start += snprintf(m + start, countof(msg) - start, "%s pid %d (%x)\n", e.process_name.c_str(), e.pid, e.pid);
				prompt.push_back(e.pid);
				break;
			}
		}
	}

	snprintf(msg + start, countof(msg) - start, "\nWould you like to terminate these processes now?");

	// add prior instances of this process
	process::id this_process_id = this_process::get_id();
	path_t this_module_path = this_module::get_path();
	process::enumerate([&](process::id id, process::id parent_id, const path_t& module_path)->bool
	{
		if (this_process_id != id && !_stricmp(this_module_path, module_path))
			kill.push_back(id);
		return true;
	});

	for (const auto& id : kill)
	{
		if (process::has_debugger_attached(id))
			continue;

		process::terminate(id, std::errc::operation_canceled);
		if (!process::wait_for(id, std::chrono::seconds(5)))
			inform.push_back(id);
	}

	msg_result r = msg_result::yes;
	if (prompt_user && !prompt.empty())
		r = msgbox(msg_type::yesno, nullptr, msgbox_title, msg);

	int retries = 10;
TryAgain:
	if (r == msg_result::yes && retries)
	{
		for (const auto& pid : prompt)
		{
			try { process::terminate(pid, 0x0D1EC0DE); }
			catch (std::system_error& e)
			{
				if (e.code().value() != std::errc::no_such_process)
				{
					path_t name;
					try { name = process::get_name(pid); }
					catch (std::exception&)
					{
						continue; // might've been a child of another process on the list, so it appears it's no longer around, thus continue.
					}

					if (prompt_user)
						r = msgbox(msg_type::yesno, nullptr, msgbox_title, "Terminating Process '%s' (%u) failed. Please close the process manually.\n\nTry Again?", name.c_str(), pid);
					else
					{
						if (0 == --retries)
							inform.push_back(pid);
					}

					if (r == msg_result::no)
						inform.push_back(pid);

					goto TryAgain;
				}
			}
		}
	}

	r = msg_result::yes;
	if (prompt_user && !inform.empty())
	{
		int start = snprintf(msg, "The following processes could not be terminated:\n");

		for (const auto& pid : inform)
			start += snprintf(msg + start, countof(msg) - start, "%s pid %d (%x)\n", process::get_name(pid), pid, pid);
		
		snprintf(msg + start, countof(msg) - start, "\nWould you like continue anyway?");
		r = msgbox(msg_type::yesno, nullptr, msgbox_title, msg);
	}

	return r == msg_result::yes;
}

}