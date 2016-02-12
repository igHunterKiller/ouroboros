// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oSystem/windows/win_error.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <comutil.h>

// ref support
inline ULONG ref_count(IUnknown* unk) { ULONG r = unk->AddRef()-1; unk->Release(); return r; }
inline void ref_reference(IUnknown* unk) { unk->AddRef(); }
inline void ref_release(IUnknown* unk) { unk->Release(); }

#define oSAFE_RELEASE(p) do { if (p) { p->Release(); p = nullptr; } } while(false)
#define oSAFE_RELEASEV(p) do { if (p) { ((IUnknown*)p)->Release(); p = nullptr; } } while(false)

namespace ouro { namespace windows {

class scoped_handle
{
	scoped_handle(scoped_handle&);
	const scoped_handle& operator=(scoped_handle&);

public:
	scoped_handle() : h(nullptr) {}
	scoped_handle(HANDLE _Handle) : h(_Handle) { oVB(h != INVALID_HANDLE_VALUE); }
	scoped_handle(scoped_handle&& _That) { operator=(std::move(_That)); }
	const scoped_handle& operator=(scoped_handle&& _That)
	{
		if (this != &_That)
		{
			close();
			h = _That.h;
			_That.h = nullptr;
		}
		return *this;
	}

	const scoped_handle& operator=(HANDLE _That)
	{
		close();
		h = _That;
	}

	~scoped_handle() { close(); }

	operator HANDLE() { return h; }

private:
	HANDLE h;
	void close() { if (h && h != INVALID_HANDLE_VALUE) { ::CloseHandle(h); h = nullptr; } }
};

}}

// primarily intended for id classes
template<typename T> DWORD asdword(const T& _ID) { return *((DWORD*)&_ID); }
template<> inline DWORD asdword(const std::thread::id& _ID) { return ((_Thrd_t*)&_ID)->_Id; }
inline std::thread::id astid(DWORD _ID)
{
	_Thrd_t tid; 
	ouro::windows::scoped_handle h(OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, _ID));
	tid._Hnd = h;
	tid._Id = _ID;
	return *(std::thread::id*)&tid;
}
