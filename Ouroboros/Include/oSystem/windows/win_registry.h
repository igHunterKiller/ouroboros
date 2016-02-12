// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Read / Write info to the registry
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724897%28v=vs.85%29.aspx

#pragma once
#include <oString/fixed_string.h>

namespace ouro { namespace windows { namespace registry {

enum hkey
{
	classes_root,
	current_user,
	local_machine,
	users,
};

void delete_value(hkey _hKey, const char* _KeyPath, const char* _ValueName);
void delete_key(hkey _hKey, const char*_KeyPath, bool _Recursive = true);

// Sets the specified key's value to _pValue as a string. If the key path does 
// not exist, it is created. If _pValueName is null or "", then the (default) 
// value key.
void set(hkey _hKey, const char* _KeyPath, const char* _ValueName, const char* _Value);

// Fills _StrDestination with the specified Key's path. Returns _StrDestination
// or nullptr if the specified key does not exist.
char* get(char* _StrDestination, size_t _SizeofStrDestination, hkey _hKey, const char* _KeyPath, const char* _ValueName);
template<size_t size> char* get(char (&_StrDestination)[size], hkey _hKey, const char* _KeyPath, const char* _ValueName) { return get(_StrDestination, size, _hKey, _KeyPath, _ValueName); }
template<size_t capacity> char* get(ouro::fixed_string<char, capacity>& _StrDestination, hkey _hKey, const char* _KeyPath, const char* _ValueName) { return get(_StrDestination, _StrDestination.capacity(), _hKey, _KeyPath, _ValueName); }

template<typename T> bool get(T* _pTypedValue, hkey _hKey, const char* _KeyPath, const char* _ValueName)
{
	sstring buf;
	if (!get(buf, _Root, _KeyPath, _ValueName))
		return false;
	return from_string(_pTypedValue, buf);
}

}}}
