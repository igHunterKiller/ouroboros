// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// a simple, clean way of handling command line parameters

#pragma once

namespace ouro {

struct option
{
	// single-character version of command line switch. If '\0' then only fullname
	// can be used.
	char abbrev; 
	
	// fullname version of command line switch. If nullptr then only abbrev can be 
	// used.
	const char* fullname;

	// name of argument to pass to switch. If nullptr, then the switch takes no 
	// argument.
	const char* argname;

	// description displayed by optdoc
	const char* desc; 
};

// Similar to strtok, first call should specify argv and argc from main(), and
// subsequent calls should pass nullptr to those values. This searches through 
// the specified command line arguments and matches either "--fullname" or 
// "-abbrev" and fills *value with the value for the arg (or if argname is 
// nullptr, meaning there is no arg, then *value gets the fullname or nullptr if 
// the options does not exist.)
// returns:
// if there is a match, the abbrev value
// 0 for no more matches
// ' ' for regular arguments (non-option values)
// '?' for an unrecognized option (error condition)
// ':' if there was a missing optionvalue (error condition)
char opttok(const char** out_value, int argc, const char* argv[], const option* options, size_t num_options);
inline char opttok(const char** argv) { return opttok(argv, 0, nullptr, nullptr, 0); }
template<size_t size> char opttok(const char** out_value, int argc, const char* argv[], const option (&options)[size]) { return opttok(out_value, argc, argv, options, size); }

// Prints documentation for the specified options to the specified buffer.
char* optdoc(char* dst, size_t dst_size, const char* app_name, const option* options, size_t num_options, const char* loose_parameters = "");
template<size_t size> char* optdoc(char* dst, size_t dst_size, const char* app_name, const option (&options)[size], const char* loose_parameters = "") { return optdoc(dst, dst_size, app_name, options, size, loose_parameters); }
template<size_t capacity> char* optdoc(char (&dst)[capacity], const char* app_name, const option* options, size_t s_num_options, const char* loose_parameters = "") { return optdoc(dst, capacity, app_name, options, s_num_options, loose_parameters); }
template<size_t capacity, size_t size> char* optdoc(char (&dst)[capacity], const char* app_name, const option (&options)[size], const char* loose_parameters = "") { return optdoc(dst, capacity, app_name, options, size, loose_parameters); }

}
