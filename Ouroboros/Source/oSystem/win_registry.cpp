// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/endian.h>
#include <oCore/finally.h>
#include <oSystem/windows/win_registry.h>
#include <oSystem/windows/win_error.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro {
	namespace windows {
		namespace registry {

// The return values of Reg* is NOT an HRESULT, but can be parsed in the same
// manner. FAILED() does not work because the error results for Reg* are >0.

static HKEY sRoots[] = { HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_PERFORMANCE_DATA, HKEY_PERFORMANCE_TEXT, HKEY_PERFORMANCE_NLSTEXT, HKEY_CURRENT_CONFIG, HKEY_DYN_DATA, HKEY_CURRENT_USER_LOCAL_SETTINGS };

void delete_value(hkey _hKey, const char* _KeyPath, const char* _ValueName)
{
	path_string KP;
	replace(KP, _KeyPath, "/", "\\");
	HKEY hKey = nullptr;
	oV(RegOpenKeyEx(sRoots[_hKey], KP, 0, KEY_ALL_ACCESS, &hKey));
	oFinally { RegCloseKey(hKey); };
	oV(RegDeleteValue(hKey, _ValueName));
}

void delete_key(hkey _hKey, const char* _KeyPath, bool _Recursive)
{
	path_string KP;
	replace(KP, _KeyPath, "/", "\\");
	long err = RegDeleteKey(sRoots[_hKey], KP);
	if (err)
	{
		if (!_Recursive)
			oV(err);

		HKEY hKey = nullptr;
		oV(RegOpenKeyEx(sRoots[_hKey], KP, 0, KEY_READ, &hKey));
		oFinally { RegCloseKey(hKey); };
		if (KP[KP.length()-1] != '\\')
			strlcat(KP, "\\");
		size_t KPLen = KP.length();
		DWORD dwSize = DWORD(KP.capacity() - KPLen);
		err = RegEnumKeyEx(hKey, 0, &KP[KPLen], &dwSize, nullptr, nullptr, nullptr, nullptr);
		while (!err)
		{
			delete_key(_hKey, KP, _Recursive);
			DWORD dwSize = DWORD(KP.capacity() - KPLen);
			err = RegEnumKeyEx(hKey, 0, &KP[KPLen], &dwSize, nullptr, nullptr, nullptr, nullptr);
		}

		KP[KPLen] = 0;
		// try again to delete original
		oV(RegDeleteKey(sRoots[_hKey], KP));
	}
}

void set(hkey _hKey, const char* _KeyPath, const char* _ValueName, const char* _Value)
{
	path_string KP;
	replace(KP, _KeyPath, "/", "\\");
	HKEY hKey = nullptr;
	oV(RegCreateKeyEx(sRoots[_hKey], KP, 0, 0, 0, KEY_SET_VALUE, 0, &hKey, 0));
	oFinally { RegCloseKey(hKey); };
	oV(RegSetValueEx(hKey, _ValueName, 0, REG_SZ, (BYTE*)_Value, (DWORD) (strlen(_Value) + 1))); // +1 for null terminating, the null character must also be counted
}

char* get(char* _StrDestination, size_t _SizeofStrDestination, hkey _hKey, const char* _KeyPath, const char* _ValueName)
{
	path_string KP;
	replace(KP, _KeyPath, "/", "\\");

	DWORD type = 0;
	if (FAILED(RegGetValue(sRoots[_hKey], KP, _ValueName, RRF_RT_ANY, &type, _StrDestination, (LPDWORD)&_SizeofStrDestination)))
		return nullptr;

	switch (type)
	{
		case REG_SZ:
			break;

		case REG_DWORD_LITTLE_ENDIAN: // REG_DWORD
			to_string(_StrDestination, _SizeofStrDestination, *(unsigned int*)_StrDestination);
			break;

		case REG_DWORD_BIG_ENDIAN:
			to_string(_StrDestination, _SizeofStrDestination, endian_swap(*(unsigned int*)_StrDestination));
			break;

		case REG_QWORD:
			to_string(_StrDestination, _SizeofStrDestination, *(unsigned long long*)_StrDestination);
			break;

		default:
			throw std::system_error(std::errc::operation_not_supported, std::system_category());
	}

	return _StrDestination;
}

		} // namespace registry
	}
}
