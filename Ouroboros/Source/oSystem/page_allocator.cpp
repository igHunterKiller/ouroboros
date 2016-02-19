// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/page_allocator.h>
#include <oSystem/windows/win_error.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro { namespace page_allocator {

static status get_status(DWORD state)
{
	switch (state)
	{
		case MEM_COMMIT: return status::committed;
		case MEM_FREE: return status::free;
		case MEM_RESERVE: return status::reserved;
		default: oThrow(std::errc::invalid_argument, "unexpected status %u", state);
	}
}

static DWORD get_access(access access)
{
	switch (access)
	{
		case access::none: return PAGE_NOACCESS;
		case access::read_only: return PAGE_EXECUTE_READ;
		case access::read_write: return PAGE_EXECUTE_READWRITE;
		default: oThrow(std::errc::invalid_argument, "unexpected access %u", access);
	}
}

size_t pagesize()
{
	SYSTEM_INFO sysInfo = {0};
	GetSystemInfo(&sysInfo);
	return static_cast<size_t>(sysInfo.dwPageSize);
}

size_t large_pagesize()
{
	return GetLargePageMinimum();
}

range get_range(void* base)
{
	MEMORY_BASIC_INFORMATION mbi;
	#if oHAS_oASSERT
		size_t returnSize = 
	#endif
	VirtualQuery(base, &mbi, sizeof(mbi));
	oAssert(sizeof(mbi) == returnSize, "");
	range r;
	r.base = mbi.BaseAddress;
	r.size = mbi.RegionSize;
	r.status = get_status(mbi.State);
	r.read_write = (mbi.AllocationProtect & PAGE_EXECUTE_READWRITE) || (mbi.AllocationProtect & PAGE_READWRITE);
	r.is_private = mbi.Type == MEM_PRIVATE;
	return r;
}

enum class allocation_type
{
	reserve,
	reserve_read_write,
	commit,
	commit_read_write,
	reserve_and_commit,
	reserve_and_commit_read_write,
};

// This will populate the flags correctly, and adjust size to be aligned if 
// large page sizes are to be used.
static void get_allocation_type(const allocation_type& allocation_type, void* base_address, bool use_large_page_size, size_t* out_size, DWORD* out_flAllocationType, DWORD* out_dwFreeType)
{
	*out_flAllocationType = 0;
	*out_dwFreeType = 0;

	if (use_large_page_size)
	{
		if (base_address && (allocation_type == allocation_type::reserve || allocation_type == allocation_type::reserve_read_write))
			oThrow(std::errc::invalid_argument, "large page memory cannot be reserved");

		*out_flAllocationType |= MEM_LARGE_PAGES;
		*out_size = align(*out_size, large_pagesize());
	}

	switch (allocation_type)
	{
		case allocation_type::reserve_read_write: *out_flAllocationType |= MEM_WRITE_WATCH; // pass thru to allocation_type::reserve
		case allocation_type::reserve: *out_flAllocationType |= MEM_RESERVE; *out_dwFreeType = MEM_RELEASE; break;
		case allocation_type::commit_read_write: // MEM_WRITE_WATCH is not a valid option for committing
		case allocation_type::commit: *out_flAllocationType |= MEM_COMMIT; *out_dwFreeType = MEM_DECOMMIT; break;
		case allocation_type::reserve_and_commit_read_write: *out_flAllocationType |= MEM_WRITE_WATCH; // pass thru to allocation_type::reserve_and_commit
		case allocation_type::reserve_and_commit: *out_flAllocationType |= MEM_RESERVE | MEM_COMMIT; *out_dwFreeType = MEM_RELEASE | MEM_DECOMMIT; break;
	}
}

static void deallocate(void* base_address, bool uncommit_and_unreservce)
{
	DWORD flAllocationType, dwFreeType;
	size_t size = 0;
	get_allocation_type(uncommit_and_unreservce ? allocation_type::reserve_and_commit : allocation_type::commit, base_address, false, &size, &flAllocationType, &dwFreeType);
	oVB(VirtualFreeEx(GetCurrentProcess(), base_address, 0, dwFreeType));
}

static void* allocate(const allocation_type& allocation_type, void* base_address, size_t size, bool use_large_page_size)
{
	DWORD flAllocationType, dwFreeType;
	get_allocation_type(allocation_type, base_address, use_large_page_size, &size, &flAllocationType, &dwFreeType);
	DWORD flProtect = get_access(((int)allocation_type & 0x1) ? access::read_write : access::read_only);
	void* p = VirtualAllocEx(GetCurrentProcess(), base_address, size, flAllocationType, flProtect);
	if (base_address && p != base_address)
	{
		deallocate(p, allocation_type >= allocation_type::reserve_and_commit);
		oThrow(std::errc::no_buffer_space, "");
	}
	return p;
}

void* reserve(void* desired_pointer, size_t size, bool read_write)
{
	return allocate(read_write 
		? allocation_type::reserve_read_write 
		: allocation_type::reserve
		, desired_pointer
		, size
		, false);
}

void unreserve(void* _Pointer)
{
	oVB(VirtualFreeEx(GetCurrentProcess(), _Pointer, 0, MEM_RELEASE));
}

void* commit(void* base_address, size_t size, bool readwrite, bool use_large_page_size)
{
	return allocate(readwrite 
		? allocation_type::commit_read_write 
		: allocation_type::commit
		, base_address
		, size
		, use_large_page_size);
}

void decommit(void* _Pointer)
{
	oVB(VirtualFreeEx(GetCurrentProcess(), _Pointer, 0, MEM_DECOMMIT));
}

void* reserve_and_commit(void* base_address, size_t size, bool read_write, bool use_large_page_size)
{
	return allocate(read_write 
		? allocation_type::reserve_and_commit_read_write 
		: allocation_type::reserve_and_commit
		, base_address
		, size
		, use_large_page_size);
}

void set_access(void* base_address, size_t size, access access)
{
	DWORD oldPermissions = 0;
	oVB(VirtualProtect(base_address, size, get_access(access), &oldPermissions));
}

void set_pagability(void* base_address, size_t size, bool pageable)
{
	oVB(pageable ? VirtualUnlock(base_address, size) : VirtualLock(base_address, size));
}

}}
