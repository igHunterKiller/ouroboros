// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oSystem/windows/win_error.h>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <DShow.h>

// dxerr is not just a part of Windows error HRESULT handling.
#if NTDDI_VERSION < _WIN32_WINNT_WIN8
	#include <dxerr.h>
	#include <d3dx11.h>
	#pragma comment(lib, "dxerr.lib")
#endif

// Use the Windows Vista UI look. If this causes issues or the dialog not to appear, try other values from processorAchitecture { x86 ia64 amd64 * }
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace ouro {
	namespace windows {

static errno_t errno_from_hresult(HRESULT _hResult)
{
	switch (_hResult)
	{
		// From WinError.h
		case ERROR_FILE_NOT_FOUND: return ENOENT;
		case ERROR_PATH_NOT_FOUND: return ENOENT;
		case ERROR_TOO_MANY_OPEN_FILES: return EMFILE;
		case ERROR_DIR_NOT_EMPTY : return ENOTEMPTY;
		case ERROR_ACCESS_DENIED: return EACCES;
		case ERROR_INVALID_HANDLE: return ENOTRECOVERABLE;
		case ERROR_ARENA_TRASHED: return ENOTRECOVERABLE;
		case ERROR_NOT_ENOUGH_MEMORY: return ENOMEM;
		case ERROR_INVALID_BLOCK: return ENOTRECOVERABLE;
		case ERROR_BAD_ENVIRONMENT: return ENOTRECOVERABLE;
		case ERROR_BAD_FORMAT: return ENOTRECOVERABLE;
		case ERROR_INVALID_ACCESS: return EACCES;
		case ERROR_INVALID_DATA: return ENOTRECOVERABLE;
		case ERROR_OUTOFMEMORY: return ENOMEM;
		case ERROR_INVALID_DRIVE: return ENOTRECOVERABLE;
		case ERROR_CURRENT_DIRECTORY: return ENOTRECOVERABLE;
		case ERROR_NOT_SAME_DEVICE: return ENOTRECOVERABLE;
		case ERROR_NO_MORE_FILES: return ENOTRECOVERABLE;
		case ERROR_WRITE_PROTECT: return EACCES;
		case ERROR_BAD_UNIT: return ENOTRECOVERABLE;
		case ERROR_NOT_READY: return ENOTRECOVERABLE;
		case ERROR_BAD_COMMAND: return ENOTRECOVERABLE;
		case ERROR_CRC: return ENOTRECOVERABLE;
		case ERROR_BAD_LENGTH: return ENOTRECOVERABLE;
		case ERROR_SEEK: return EIO;
		case ERROR_NOT_DOS_DISK: return ENOTRECOVERABLE;
		case ERROR_WRITE_FAULT: return EFAULT;
		case ERROR_READ_FAULT: return EFAULT;
		case ERROR_SHARING_VIOLATION: return EACCES;
		case ERROR_LOCK_VIOLATION: return EACCES;
		case ERROR_WRONG_DISK: return ENOTRECOVERABLE;
		case E_NOINTERFACE: return ENOSYS;
		case E_OUTOFMEMORY: return ENOMEM;
		case E_FAIL: return ENOTRECOVERABLE;
		case E_ACCESSDENIED: return EACCES;
		case E_INVALIDARG: return EINVAL;
		case E_POINTER: return EINVAL;
		case S_OK: return 0;
		case S_FALSE: return ENOTRECOVERABLE;
		default: break;
	}
	return ENOTRECOVERABLE;
}

const char* as_string_HR(HRESULT _hResult)
{
	switch (_hResult)
	{
		// From WinError.h
		//case ERROR_INVALID_FUNCTION: return "ERROR_INVALID_FUNCTION"; // same value as S_FALSE
		case ERROR_FILE_NOT_FOUND: return "ERROR_FILE_NOT_FOUND";
		case ERROR_PATH_NOT_FOUND: return "ERROR_PATH_NOT_FOUND";
		case ERROR_TOO_MANY_OPEN_FILES: return "ERROR_TOO_MANY_OPEN_FILES";
		case ERROR_DIR_NOT_EMPTY : return "ERROR_DIR_NOT_EMPTY";
		case ERROR_ACCESS_DENIED: return "ERROR_ACCESS_DENIED";
		case ERROR_INVALID_HANDLE: return "ERROR_INVALID_HANDLE";
		case ERROR_ARENA_TRASHED: return "ERROR_ARENA_TRASHED";
		case ERROR_NOT_ENOUGH_MEMORY: return "ERROR_NOT_ENOUGH_MEMORY";
		case ERROR_INVALID_BLOCK: return "ERROR_INVALID_BLOCK";
		case ERROR_BAD_ENVIRONMENT: return "ERROR_BAD_ENVIRONMENT";
		case ERROR_BAD_FORMAT: return "ERROR_BAD_FORMAT";
		case ERROR_INVALID_ACCESS: return "ERROR_INVALID_ACCESS";
		case ERROR_INVALID_DATA: return "ERROR_INVALID_DATA";
		case ERROR_OUTOFMEMORY: return "ERROR_OUTOFMEMORY";
		case ERROR_INVALID_DRIVE: return "ERROR_INVALID_DRIVE";
		case ERROR_CURRENT_DIRECTORY: return "ERROR_CURRENT_DIRECTORY";
		case ERROR_NOT_SAME_DEVICE: return "ERROR_NOT_SAME_DEVICE";
		case ERROR_NO_MORE_FILES: return "ERROR_NO_MORE_FILES";
		case ERROR_WRITE_PROTECT: return "ERROR_WRITE_PROTECT";
		case ERROR_BAD_UNIT: return "ERROR_BAD_UNIT";
		case ERROR_NOT_READY: return "ERROR_NOT_READY";
		case ERROR_BAD_COMMAND: return "ERROR_BAD_COMMAND";
		case ERROR_CRC: return "ERROR_CRC";
		case ERROR_BAD_LENGTH: return "ERROR_BAD_LENGTH";
		case ERROR_SEEK: return "ERROR_SEEK";
		case ERROR_NOT_DOS_DISK: return "ERROR_NOT_DOS_DISK";
		case ERROR_WRITE_FAULT: return "ERROR_WRITE_FAULT";
		case ERROR_READ_FAULT: return "ERROR_READ_FAULT";
		case ERROR_SHARING_VIOLATION: return "ERROR_SHARING_VIOLATION";
		case ERROR_LOCK_VIOLATION: return "ERROR_LOCK_VIOLATION";
		case ERROR_WRONG_DISK: return "ERROR_WRONG_DISK";
		case E_NOINTERFACE: return "E_NOINTERFACE";
		case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
		case E_FAIL: return "E_FAIL";
		case E_ACCESSDENIED: return "E_ACCESSDENIED";
		case E_INVALIDARG: return "E_INVALIDARG";
		case S_OK: return "S_OK";
		case S_FALSE: return "S_FALSE";
		default: return "?";
	}
}

const char* as_string_HR_DXGI(HRESULT _hResult)
{
	switch (_hResult)
	{
		case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
		case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND";
		case DXGI_ERROR_MORE_DATA: return "DXGI_ERROR_MORE_DATA";
		case DXGI_ERROR_UNSUPPORTED: return "DXGI_ERROR_UNSUPPORTED";
		case DXGI_ERROR_DEVICE_REMOVED: return "DXGI_ERROR_DEVICE_REMOVED";
		case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
		case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
		case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING";
		case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
		case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";
		case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
		case DXGI_ERROR_NONEXCLUSIVE: return "DXGI_ERROR_NONEXCLUSIVE";
		case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
		case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED: return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
		case DXGI_ERROR_REMOTE_OUTOFMEMORY: return "DXGI_ERROR_REMOTE_OUTOFMEMORY";
		default: break;
	}
	return as_string_HR(_hResult);
}

const char* as_string_HR_DX11(HRESULT _hResult)
{
	switch (_hResult)
	{
		case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND";
		case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
		case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
		case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
		#if NTDDI_VERSION < _WIN32_WINNT_WIN8
			case D3DERR_INVALIDCALL: return "D3DERR_INVALIDCALL";
			case D3DERR_WASSTILLDRAWING: return "D3DERR_WASSTILLDRAWING";
		#endif
		default: break;
	}
	return as_string_HR_DXGI(_hResult);
}

const char* as_string_HR_VFW(HRESULT _hResult)
{
	switch (_hResult)
	{
		case VFW_E_INVALIDMEDIATYPE: return "VFW_E_INVALIDMEDIATYPE";
		case VFW_E_NOT_CONNECTED: return "VFW_E_NOT_CONNECTED";
		case VFW_E_NOT_STOPPED: return "VFW_E_NOT_STOPPED";
		case VFW_E_WRONG_STATE: return "VFW_E_WRONG_STATE";
		default: break;
	}
	return as_string_HR(_hResult);
}

class scoped_local_alloc
{
public:
	scoped_local_alloc(void* _Pointer) : p(_Pointer) {}
	~scoped_local_alloc() { if (p) LocalFree(p); }
private:
	void* p;
};

static std::string message(HRESULT _hResult)
{
	std::string msg;
	char* pMessage = nullptr;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM, 0, static_cast<DWORD>(_hResult), 0, (LPTSTR)&pMessage, 16, 0);
	scoped_local_alloc Message(pMessage);

	if (!pMessage || !*pMessage || !memcmp(pMessage, "???", 3))
	{
		char buf[512];
		const char* s = as_string_HR_DX11(_hResult);
		if (*s == '?') s = as_string_HR_VFW(_hResult);
		#if NTDDI_VERSION < _WIN32_WINNT_WIN8
			if (*s == '?') s = DXGetErrorStringA(_hResult);
		#endif
		if (*s == '?') { _snprintf_s(buf, sizeof(buf), "unrecognized error code 0x%08x", _hResult); s = buf; }
		else msg = s;
	}
	else
		msg = pMessage;

	return msg;
}

std::error_code make_error_code(long _hResult)
{
	errno_t e = errno_from_hresult((HRESULT)_hResult);
	if (e && e != ENOTRECOVERABLE)
		return std::error_code(e, std::generic_category());
	return std::error_code(_hResult, category()); 
}

std::error_condition make_error_condition(long _hResult)
{
	errno_t e = errno_from_hresult((HRESULT)_hResult);
	if (e && e != ENOTRECOVERABLE)
		return std::error_condition(e, std::generic_category());
	return std::error_condition(_hResult, category());
}

class category_impl : public std::error_category
{
public:
	const char* name() const { return "windows"; }
	std::string message(int _ErrCode) const
	{
		errno_t e = errno_from_hresult((HRESULT)_ErrCode);
		if (e && e != ENOTRECOVERABLE)
			return std::generic_category().message(e);
		return windows::message((HRESULT)_ErrCode);
	}
};

const std::error_category& category()
{
	static windows::category_impl sInstance;
	return sInstance;
}

	}
}
