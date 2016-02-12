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

std::error_code make_error_code(long _hResult);
std::error_condition make_error_condition(long _hResult);

class error : public std::system_error
{
public:
	error(const error& _That) : std::system_error(_That) {}
	error() : system_error(make_error_code(GetLastError()), make_error_code(GetLastError()).message()) { trace(); }
	error(const char* _Message) : system_error(make_error_code(GetLastError()), _Message) { trace(); }
	error(long _hResult) : system_error(make_error_code(_hResult), make_error_code(_hResult).message()) { trace(); }
	error(long _hResult, const char* _Message) : system_error(make_error_code(_hResult), _Message) { trace(); }
	error(long _hResult, const std::string& _Message) : system_error(make_error_code(_hResult), _Message) { trace(); }
private:
	void trace() { char msg[1024]; _snprintf_s(msg, sizeof(msg), "\nouro::windows::error: 0x%08x: %s\n\n", code().value(), what()); OutputDebugStringA(msg); }
};

}}

// For Windows API that returns an HRESULT, this captures that value and throws 
// on failure.
#define oV(_HRWinFn) do { long HR__ = _HRWinFn; if (HR__) throw ouro::windows::error(HR__); } while(false)

// For Windows API that returns BOOL and uses GetLastError(), this throws on 
// failure.
#define oVB(_BoolWinFn) do { if (!(_BoolWinFn)) throw ::ouro::windows::error(); } while(false)

#define oVB_MSG(_BoolWinFn, _Format, ...) do { if (!(_BoolWinFn)) { char msg[1024]; _snprintf_s(msg, sizeof(msg), _Format, ## __VA_ARGS__); throw ::ouro::windows::error(msg); } } while(false)
