// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Interface for working with monitors/displays

#pragma once
#include <oString/path.h>
#include <oCore/version.h>

namespace ouro { namespace module {

enum class type
{
	unknown,
	app,
	dll,
	lib,
	font_unknown,
	font_raster,
	font_truetype,
	font_vector,
	virtual_device,
	drv_unknown,
	drv_comm,
	drv_display,
	drv_installable,
	drv_keyboard,
	drv_language,
	drv_mouse,
	drv_network,
	drv_printer,
	drv_sound,
	drv_system,
};

class id
{
public:
	id() : handle_(nullptr) {}

	bool operator==(const id& that) const { return handle_ == that.handle_; }
	bool operator!=(const id& that) const { return !(*this == that); }
	operator bool() const { return !!handle_; }

private:
	void* handle_;
};

struct info
{
	info()
		: type(type::unknown)
		, is_64bit_binary(false)
		, is_debug(false)
		, is_prerelease(false)
		, is_patched(false)
		, is_private(false)
		, is_special(false)
	{}

	version_t version;
	mstring company;
	mstring description;
	mstring product_name;
	mstring copyright;
	mstring original_filename;
	mstring comments;
	mstring private_message;
	mstring special_message;
	type type;
	bool is_64bit_binary;
	bool is_debug;
	bool is_prerelease;
	bool is_patched;
	bool is_private;
	bool is_special;
};

// similar to dlopen, et al.
id open(const path_t& path);
void close(id module_id);
void* sym(id module_id, const char* symbol_name);

// convenience function that given an array of function names will fill an array 
// of the same size with the resolved symbols. If a class is defined that has a 
// run of members defined for the functions, then the address of the first 
// function can be passed as the array to quickly fill an interface of typed
// functions ready for use.
void link(id module_id, const char** interface_function_names, void** interfaces, size_t num_interfaces);
template<size_t size> void link(id module_id, const char* (&interface_function_names)[size], void** interfaces) { link(module_id, interface_function_names, interfaces, size); }

inline id link(const path_t& path, const char** interface_function_names, void** interfaces, size_t num_interfaces) { id ID = open(path); link(ID, interface_function_names, interfaces, num_interfaces); return ID; }
template<size_t size> id link(const path_t& path, const char* (&interface_function_names)[size], void** interfaces) { return link(path, interface_function_names, interfaces, size); }

path_t get_path(id module_id);

// given an address in a module, retreive that module's handle
// fit for name(), sym() or close() APIs.
id get_id(const void* symbol);

info get_info(const path_t& path);
info get_info(id module_id);

// fills out_paths with the full paths of all dependency modules and returns the number of 
// valid ones. The count can be larger than max_num_paths, so check for that as an indication
// that more capacity should be passed in. The module itself is always the 0th dependency.
uint32_t enum_dependencies(const char* module_path, path_t* out_paths, uint32_t max_num_paths);
template<uint32_t size> uint32_t enum_dependencies(const char* module_path, path_t (&out_paths)[size]) { return enum_dependencies(module_path, out_paths, size); }

}

namespace this_module {

module::id get_id();
path_t get_path();
module::info get_info();

}}
