// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// A vectored exception handler that redirects some other handlers to the main 
// exception path and calls to the handler when an exception occurs.

#pragma once
#include <oString/path.h>
#include <string>

namespace ATL { struct CAtlException; }
class _com_error;

namespace ouro { namespace windows { namespace exception {

enum class type { unknown, std, com, atl, count };

struct cpp_exception
{
	cpp_exception() : type(type::unknown), type_name(""), what("") { void_exception = nullptr; }

	type                  type;
	const char*           type_name;
	std::string           what;
	union
	{
		std::exception*     std_exception;
		_com_error*         com_error;
		ATL::CAtlException* atl_exception;
		void*               void_exception;
	};
};

typedef void(*handler_fn)(const char* message, const cpp_exception& cpp_exception, uintptr_t exception_context, void* user);

void set_handler(handler_fn handler, void* user);

// Specify a directory where mini/full dumps will be written. Dump filenames 
// will be generated with a timestamp at the time of exception.
void mini_dump_path(const path_t& mini_dump_path);
const path_t& mini_dump_path();

void full_dump_path(const path_t& full_dump_path);
const path_t& full_dump_path();

void post_dump_command(const char* command);
const char* post_dump_command();

void prompt_after_dump(bool prompt);
bool prompt_after_dump();

// Set the enable state of debug CRT asserts and errors
void enable_dialogs(bool enable);

}}}
