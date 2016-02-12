// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// parse a single command line into an argc, argv style array

#pragma once

namespace ouro {

// parses a string of command lines into an array of tokens. malloc is used if 
// allocate is nullptr. app_path can be nullptr, but if not it is copied to the
// 0th element (consistent with argv-style arguments). Use this as an ASCII 
// version of Window's CommandLineToArgvW.
const char** argtok(void* (*allocate)(size_t), const char* app_path, const char* command_line, int* out_num_args);

}
