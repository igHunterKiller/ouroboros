// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/windows/win_version.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <VersionHelpers.h>

namespace ouro {

template<> const char* as_string<windows::version>(const windows::version& v)
{
	switch (v)
	{
		case windows::version::win2000: return "Windows 2000";
		case windows::version::xp: return "Windows XP";
		case windows::version::xp_sp1: return "Windows XP SP1";
		case windows::version::xp_sp2: return "Windows XP SP2";
		case windows::version::xp_sp3: return "Windows XP SP3";
		case windows::version::xp_pro_64bit: return "Windows XP Pro 64-bit";
		case windows::version::server_2003: return "Windows Server 2003";
		case windows::version::home_server: return "Windows Home Server";
		case windows::version::server_2003r2: return "Windows Server 2003R2";
		case windows::version::vista: return "Windows Vista";
		case windows::version::server_2008: return "Windows Server 2008";
		case windows::version::server_2008r2: return "Windows Server 2008R2";
		case windows::version::win7: return "Windows 7";
		case windows::version::win7_sp1: return "Windows 7 SP1";
		case windows::version::win8: return "Windows 8";
		case windows::version::server_2012: "Windows Server 2012";
		case windows::version::win8_1: return "Windows 8.1";
		case windows::version::server_2012_sp1: "Windows Server 2012 SP1";
		case windows::version::win10: "Windows 10";
		case windows::version::unknown:
		default: break;
	}

	return "?";
}

	namespace windows {

version get_version()
{
#if (_MSC_VER >= oVS2015_VER)

	bool is_server = IsWindowsServer();

	//if (IsWindows10OrGreater())             return is_server ? windows::version::win10             : windows::version::win10;
	if (IsWindows8Point1OrGreater())        return is_server ? windows::version::server_2012_sp1   : windows::version::win8_1;
	if (IsWindows8OrGreater())              return is_server ? windows::version::server_2012       : windows::version::win8;
	if (IsWindows7SP1OrGreater())           return is_server ? windows::version::server_2008r2_sp1 : windows::version::win7_sp1;
	if (IsWindows7OrGreater())              return is_server ? windows::version::server_2008r2     : windows::version::win7;
	if (IsWindowsVistaSP2OrGreater())       return is_server ? windows::version::server_2008_sp2   : windows::version::vista_sp2;
	if (IsWindowsVistaSP1OrGreater())       return is_server ? windows::version::server_2008_sp1   : windows::version::vista_sp1;
	if (IsWindowsVistaOrGreater())          return is_server ? windows::version::server_2008       : windows::version::vista;
	if (IsWindowsXPSP3OrGreater())          return is_server ? windows::version::xp								 : windows::version::xp;
	if (IsWindowsXPSP2OrGreater())		      return is_server ? windows::version::xp_sp1						 : windows::version::xp_sp1;
	if (IsWindowsXPSP1OrGreater())		      return is_server ? windows::version::xp_sp2						 : windows::version::xp_sp2;
	if (IsWindowsXPOrGreater())				      return is_server ? windows::version::xp_sp3						 : windows::version::xp_sp3;
	if (IsWindowsVersionOrGreater(5, 2, 0)) return is_server ? windows::version::server_2003       : windows::version::xp_pro_64bit;
	if (IsWindowsVersionOrGreater(5, 0, 0)) return is_server ? windows::version::win2000           : windows::version::win2000;

#else
	OSVERSIONINFOEX osvi;
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	if (GetVersionEx((OSVERSIONINFO*)&osvi))
	{
		if (osvi.dwMajorVersion == 6)
		{
			if (osvi.dwMinorVersion == 2)
			{
				if (osvi.wServicePackMajor == 1)
					return osvi.wProductType == VER_NT_WORKSTATION ? version::win8_1 : version::server_2012_sp1;
				else if (osvi.wServicePackMajor == 0)
					return osvi.wProductType == VER_NT_WORKSTATION ? version::win8 : version::server_2012;
			}

			else if (osvi.dwMinorVersion == 1)
			{
				if (osvi.wServicePackMajor == 0)
					return osvi.wProductType == VER_NT_WORKSTATION ? version::win7 : version::server_2008r2;
				else if (osvi.wServicePackMajor == 1)
					return osvi.wProductType == VER_NT_WORKSTATION ? version::win7_sp1 : version::server_2008r2_sp1;
			}
			else if (osvi.dwMinorVersion == 0)
			{
				if (osvi.wServicePackMajor == 2)
					return osvi.wProductType == VER_NT_WORKSTATION ? version::vista_sp2 : version::server_2008_sp2;
				else if (osvi.wServicePackMajor == 1)
					return osvi.wProductType == VER_NT_WORKSTATION ? version::vista_sp1 : version::server_2008_sp1;
				else if (osvi.wServicePackMajor == 0)
					return osvi.wProductType == VER_NT_WORKSTATION ? version::vista : version::server_2008;
			}
		}

		else if (osvi.dwMajorVersion == 5)
		{
			if (osvi.dwMinorVersion == 2)
			{
				SYSTEM_INFO si;
				GetSystemInfo(&si);
				if ((osvi.wProductType == VER_NT_WORKSTATION) && (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64))
					return version::xp_pro_64bit;
				else if (osvi.wSuiteMask & 0x00008000 /*VER_SUITE_WH_SERVER*/)
					return version::home_server;
				else
					return GetSystemMetrics(SM_SERVERR2) ? version::server_2003r2 : version::server_2003;
			}

			else if (osvi.dwMinorVersion == 1)
				return version::xp;
			else if (osvi.dwMinorVersion == 0)
				return version::win2000;
		}
	}
#endif

	return version::unknown;
}

}}
