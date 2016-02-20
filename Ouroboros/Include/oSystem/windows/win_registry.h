// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Read / Write info to the registry
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724897%28v=vs.85%29.aspx

#pragma once
#include <oString/fixed_string.h>

namespace ouro { namespace windows { namespace registry {

enum class hkey
{
	classes_root,
	current_user,
	local_machine,
	users,
};

void delete_value(hkey hkey, const char* keypath, const char* value_name);
void delete_key(hkey hkey, const char*keypath, bool recursive = true);

// Sets the specified key's value to _pValue as a string. If the key path does 
// not exist, it is created. If _pValueName is null or "", then the (default) 
// value key.
void set(hkey hkey, const char* keypath, const char* value_name, const char* value);

// Fills dst with the specified Key's path. Returns dst
// or nullptr if the specified key does not exist.
char* get(char* dst, size_t dst_size, hkey hkey, const char* keypath, const char* value_name);
template<size_t size> char* get(char (&dst)[size], hkey hkey, const char* keypath, const char* value_name) { return get(dst, size, hkey, keypath, value_name); }
template<size_t capacity> char* get(ouro::fixed_string<char, capacity>& dst, hkey hkey, const char* keypath, const char* value_name) { return get(dst, dst.capacity(), hkey, keypath, value_name); }

template<typename T> bool get(T* p_typed_value, hkey hkey, const char* keypath, const char* value_name)
{
	sstring buf;
	if (!get(buf, _Root, keypath, value_name))
		return false;
	return from_string(p_typed_value, buf);
}

}}}
