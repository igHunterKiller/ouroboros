// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <system_error>
#include <cstdio>

extern "C" {

#ifdef _WIN32
	#define WINERR_APICALL __stdcall
#else
	#define WINERR_APICALL
#endif

void __declspec(dllimport) WINERR_APICALL OutputDebugStringA(const char*);
unsigned long __declspec(dllimport) WINERR_APICALL GetLastError();

} // extern "C"

namespace ouro { namespace windows {

const std::error_category& category();

std::error_code make_error_code(long hresult);
std::error_condition make_error_condition(long hresult);

class error : public std::system_error
{
public:
	error(const error& that)                    : system_error(that)                                                                       {}
	error()                                     : system_error(make_error_code(GetLastError()), make_error_code(GetLastError()).message()) { trace(); }
	error(const char* msg)                      : system_error(make_error_code(GetLastError()), msg)                                       { trace(); }
	error(long hresult)                         : system_error(make_error_code(hresult), make_error_code(hresult).message())               { trace(); }
	error(long hresult, const char* msg)        : system_error(make_error_code(hresult), msg)                                              { trace(); }
	error(long hresult, const std::string& msg) : system_error(make_error_code(hresult), msg)                                              { trace(); }
private:
	void trace() { char msg[1024]; _snprintf_s(msg, sizeof(msg), "\nouro::windows::error: 0x%08x: %s\n\n", code().value(), what()); OutputDebugStringA(msg); }
};

}}

// For Windows API that returns an HRESULT, this captures that value and throws on failure.
#define oV(hr_fn) do { long HR__ = hr_fn; if (HR__) throw ouro::windows::error(HR__); } while(false)
#define oVB(bool_fn) do { if (!(bool_fn)) throw ::ouro::windows::error(); } while(false)
#define oVB_MSG(bool_fn, fmt, ...) do { if (!(bool_fn)) { char msg[1024]; _snprintf_s(msg, sizeof(msg), fmt, ## __VA_ARGS__); throw ::ouro::windows::error(msg); } } while(false)

// Nothrow versions
#define oV_NOTHROW(hr_fn) do { long HR__ = hr_fn; if (HR__) { ::ouro::windows::error err(HR__); __debugbreak(); } } while(false)
#define oVB_NOTHROW(bool_fn) do { if (!(bool_fn)) { ::ouro::windows::error err; __debugbreak(); } } while(false)
#define oVB_MSG_NOTHROW(bool_fn, fmt, ...) do { if (!(bool_fn)) { char msg[1024]; _snprintf_s(msg, sizeof(msg), fmt, ## __VA_ARGS__); ::ouro::windows::error err(msg); __debugbreak(); } } while(false)

