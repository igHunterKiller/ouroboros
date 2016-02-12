// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Accumulate a list of processes that should be shut down and then
// shut them down with user prompts along the way.

#pragma once

#include <oSystem/process.h>
#include <vector>

namespace ouro { 

class process_killer
{
public:
	enum killtype_t
	{
		auto_kill,
		prompt_to_kill,
		inform_to_kill,
	};

	struct entry_t
	{
		const char* process_name;
		killtype_t type;
	};

	void add(const entry_t& entry);
	void add(const entry_t* processes, uint32_t num_processes) { for (uint32_t i = 0; i < num_processes; i++) add(processes[i]); }
	void add(const killtype_t& kill_type, const char* process_name) { entry_t e; e.type = kill_type; e.process_name = process_name; add(e); }
	template<size_t size> void add(const entry_t (&processes)[size]) { add(processes, size); }

	bool kill(const char* msgbox_title, bool prompt_user = true);

private:
	struct internal_entry_t
	{
		internal_entry_t() : type(auto_kill) {}

		mstring process_name;
		killtype_t type;
		process::id pid;
	};

	std::vector<internal_entry_t> entries;
	std::vector<process::id> kill_entries;
	std::vector<process::id> prompt_entries;
	std::vector<process::id> inform_entries;
};

}
