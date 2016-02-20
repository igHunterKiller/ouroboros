// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/endian.h>
#include <oCore/finally.h>
#include <oSystem/windows/win_registry.h>
#include <oSystem/windows/win_error.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro { namespace windows { namespace registry {

// The return values of Reg* is NOT an HRESULT, but can be parsed in the same
// manner. FAILED() does not work because the error results for Reg* are >0.

static HKEY sRoots[] = { HKEY_CLASSES_ROOT, HKEY_CURRENT_USER, HKEY_LOCAL_MACHINE, HKEY_USERS, HKEY_PERFORMANCE_DATA, HKEY_PERFORMANCE_TEXT, HKEY_PERFORMANCE_NLSTEXT, HKEY_CURRENT_CONFIG, HKEY_DYN_DATA, HKEY_CURRENT_USER_LOCAL_SETTINGS };

void delete_value(hkey hkey, const char* keypath, const char* value_name)
{
	path_string KP;
	replace(KP, keypath, "/", "\\");
	HKEY hk = nullptr;
	oV(RegOpenKeyEx(sRoots[(int)hkey], KP, 0, KEY_ALL_ACCESS, &hk));
	oFinally { RegCloseKey(hk); };
	oV(RegDeleteValue(hk, value_name));
}

void delete_key(hkey hkey, const char* keypath, bool recursive)
{
	path_string KP;
	replace(KP, keypath, "/", "\\");
	long err = RegDeleteKey(sRoots[(int)hkey], KP);
	if (err)
	{
		if (!recursive)
			oV(err);

		HKEY hk = nullptr;
		oV(RegOpenKeyEx(sRoots[(int)hkey], KP, 0, KEY_READ, &hk));
		oFinally { RegCloseKey(hk); };
		if (KP[KP.length()-1] != '\\')
			strlcat(KP, "\\");
		size_t KPLen = KP.length();
		DWORD dwSize = DWORD(KP.capacity() - KPLen);
		err = RegEnumKeyEx(hk, 0, &KP[KPLen], &dwSize, nullptr, nullptr, nullptr, nullptr);
		while (!err)
		{
			delete_key(hkey, KP, recursive);
			DWORD dwKeySize = DWORD(KP.capacity() - KPLen);
			err = RegEnumKeyEx(hk, 0, &KP[KPLen], &dwKeySize, nullptr, nullptr, nullptr, nullptr);
		}

		KP[KPLen] = 0;
		// try again to delete original
		oV(RegDeleteKey(sRoots[(int)hkey], KP));
	}
}

void set(hkey hkey, const char* keypath, const char* value_name, const char* _Value)
{
	path_string KP;
	replace(KP, keypath, "/", "\\");
	HKEY hk = nullptr;
	oV(RegCreateKeyEx(sRoots[(int)hkey], KP, 0, 0, 0, KEY_SET_VALUE, 0, &hk, 0));
	oFinally { RegCloseKey(hk); };
	oV(RegSetValueEx(hk, value_name, 0, REG_SZ, (BYTE*)_Value, (DWORD) (strlen(_Value) + 1))); // +1 for null terminating, the null character must also be counted
}

char* get(char* dst, size_t dst_size, hkey hkey, const char* keypath, const char* value_name)
{
	path_string KP;
	replace(KP, keypath, "/", "\\");

	DWORD type = 0;
	if (FAILED(RegGetValue(sRoots[(int)hkey], KP, value_name, RRF_RT_ANY, &type, dst, (LPDWORD)&dst_size)))
		return nullptr;

	switch (type)
	{
		case REG_SZ:
			break;

		case REG_DWORD_LITTLE_ENDIAN: // REG_DWORD
			to_string(dst, dst_size, *(unsigned int*)dst);
			break;

		case REG_DWORD_BIG_ENDIAN:
			to_string(dst, dst_size, endian_swap(*(unsigned int*)dst));
			break;

		case REG_QWORD:
			to_string(dst, dst_size, *(unsigned long long*)dst);
			break;

		default:
			oThrow(std::errc::operation_not_supported, "");
	}

	return dst;
}

}}}
