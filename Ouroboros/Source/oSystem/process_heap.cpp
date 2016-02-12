// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oSystem/process_heap.h>
#include <oSystem/debugger.h>
#include <oSystem/module.h>
#include <oSystem/process.h>
#include <oSystem/system.h>
#include <oCore/guid.h>
#include <oMemory/fnv1a.h>
#include <oSystem/windows/win_util.h>
#include <mutex>
#include <thread>

using namespace std;

namespace ouro {
	namespace process_heap {

inline size_t hash(const char* _Name, process_heap::scope _Scope)
{
	size_t h = fnv1a<size_t>(_Name);
	if (_Scope == process_heap::per_thread)
	{
		std::thread::id id = std::this_thread::get_id();
		h = fnv1a<size_t>(&id, sizeof(thread::id), h);
	}
	return h;
}

class context
{
public:
	static context& singleton();

	void reference();
	void release();

	void* allocate(size_t _Size, size_t _Align = 64);
	void deallocate(void* _Pointer);
	void internal_deallocate(void* _Pointer);

	bool find_or_allocate(size_t _Size
		, const char* _Name
		, scope _Scope
		, tracking _Tracking
		, const std::function<void(void* _Pointer)>& _PlacementConstructor
		, const std::function<void(void* _Pointer)>& _Destructor
		, void** _pPointer);

	bool find(const char* _Name, scope _Scope, void** _pPointer);

	void exit_thread();

	void collect_garbage();

	void report();

private:
	context();

	static const size_t max_stack_depth = 32;
	static const size_t std_bind_internal_offset = 5; // number of symbols internal to std::bind to skip
	static bool valid;
	static context* sAtExitInstance;

	HANDLE hHeap;
	size_t DtorOrdinal;
	recursive_mutex Mutex;
	int RefCount;

	struct mapped_file
	{
		context* instance;
		guid_t guid;
		process::id pid;
	};
	
	struct entry
	{
		entry()
			: pointer(nullptr)
			, scope(process_heap::per_process)
			, tracking(process_heap::leak_tracked)
			, num_stack_entries(0)
			, dtor_ordinal(0)
		{
			InitializeSRWLock(&mutex);
		}

		// compare so that most recent goes first in list and eldest goes last (LIFO)
		bool operator<(const entry& _That) const
		{
			return _That.dtor_ordinal < dtor_ordinal;
		}

		// The allocated pointer
		void* pointer;

		// thread id of pointer initialization and where deinitialization will 
		// probably take place
		std::thread::id init;
		sstring name;
		enum scope scope;
		enum tracking tracking;
		debugger::symbol_t stack[max_stack_depth];
		size_t num_stack_entries;
		size_t dtor_ordinal;
		std::function<void(void* _Pointer)> dtor;
		SRWLOCK mutex; // std::mutex can't support copy or move, so need to use platform type directly

		void lock() { AcquireSRWLockExclusive(&mutex); }
		void unlock() { ReleaseSRWLockExclusive(&mutex); }
		void lock_shared() { AcquireSRWLockShared(&mutex); }
		void unlock_shared() { ReleaseSRWLockShared(&mutex); }
	};

	struct matches
	{
		matches(void* _Pointer) : Pointer(_Pointer) {}
		bool operator()(const std::pair<size_t, entry>& _Entry) { return _Entry.second.pointer == Pointer; }
		void* Pointer;
	};

	// Main container of all pointers allocated from the process heap
	typedef std::map<size_t, entry, std::less<size_t>, process_heap::std_allocator<std::pair<size_t, entry>>> container_t;
	container_t Pointers;

	static void report_footer(size_t _NumLeaks);
	static void at_exit();
};

context::context()
	: hHeap(GetProcessHeap())
	, RefCount(1)
	, DtorOrdinal(0)
{
	// From oGSReport.cpp: This touches the CPP file ensuring our __report_gsfailure is installed
	extern void oGSReportInstaller();
	oGSReportInstaller();

	sAtExitInstance = this;
	atexit(at_exit);
	valid = true;

#if 0
	path_t modulePath = this_module::get_path();
	lstring buf;
	mstring exec;
	snprintf(buf, "========== Ouro Process Heap initialized at 0x%p in %s %s ==========\n", this, modulePath.c_str(), system::exec_path(exec));
	debugger::print(buf);
#endif
}

bool context::valid = false;
context* context::sAtExitInstance = nullptr;

void context::report_footer(size_t _NumLeaks)
{
	char buf[512];
	char exec[128];
	snprintf(buf, "========== Ouro Process Heap Leak Report: %u Leaks %s ==========\n", _NumLeaks, system::exec_path(exec));
	debugger::print(buf);
}

void context::at_exit()
{
	context::singleton().exit_thread();

	if (valid)
	{
		context::singleton().collect_garbage();
		context::singleton().report();
	}
	else
		report_footer(0);
}

void context::collect_garbage()
{
	// Free any allocations flagged for auto-deallocations.
	std::vector<void*> frees;
	frees.reserve(Pointers.size());

	for (auto& pair : Pointers)
		if (pair.second.tracking == process_heap::garbage_collected)
			frees.push_back(pair.second.pointer);

	// in LIFO order
	std::sort(std::begin(frees), std::end(frees), std::greater<const void*>());

	for (void* p : frees)
		deallocate(p);
}

void context::reference()
{
	RefCount++;
}

void context::release()
{
	if (--RefCount == 0)
	{
		valid = false;
		this->~context();
		VirtualFreeEx(GetCurrentProcess(), this, 0, MEM_RELEASE);
	}
}

void* context::allocate(size_t _Size, size_t _Align)
{
	size_t padded = _Size + sizeof(void*) + _Align - 1;
	void* unaligned = HeapAlloc(hHeap, 0, padded);
	void* aligned = align((char*)unaligned + sizeof(void*), _Align);
	((void**)aligned)[-1] = unaligned;
	return aligned;
}

void context::internal_deallocate(void* _Pointer)
{
	void* unaligned = ((void**)_Pointer)[-1];
	HeapFree(hHeap, 0, unaligned);
}

void context::deallocate(void* _Pointer)
{
	bool DoRelease = false;
	{
		lock_guard<recursive_mutex> lock(Mutex);

		if (valid)
		{
			auto it = std::find_if(std::begin(Pointers), std::end(Pointers), matches(_Pointer));
			if (it == std::end(Pointers))
				internal_deallocate(_Pointer);
			else
			{
				// The order here is critical, we need to tell the heap to deallocate 
				// first, remove it from the list, exit the mutex and then release. If 
				// we release first we risk destroying the heap and then still needing 
				// to access it.
				if (it->second.dtor)
					it->second.dtor(it->second.pointer);
				internal_deallocate(it->second.pointer);
				Pointers.erase(it);
				DoRelease = true;
			}
		}

		else
		{
			// shutting down so this allocation must be from pPointers itself, so just 
			// free it
			internal_deallocate(_Pointer);
		}
	}

	if (DoRelease)
		release();
}

void context::exit_thread()
{
	fixed_vector<void*, 32> allocs;
	{
		std::thread::id tid = std::this_thread::get_id();
		lock_guard<recursive_mutex> lock(Mutex);
		for (container_t::const_iterator it = Pointers.begin(); it != Pointers.end(); ++it)
			if (it->second.scope == process_heap::per_thread && tid == it->second.init)
				allocs.push_back(it->second.pointer);
	}

	for (void* p : allocs)
		context::deallocate(p);
}

bool context::find_or_allocate(size_t _Size
	, const char* _Name
	, scope _Scope
	, tracking _Tracking
	, const std::function<void(void* _Pointer)>& _PlacementConstructor
	, const std::function<void(void* _Pointer)>& _Destructor
	, void** _pPointer)
{
	if (!_Size || !_pPointer)
		throw std::invalid_argument("invalid argument");
	size_t h = hash(_Name, _Scope);
	bool Allocated = false;
	bool CallCtor = false;

	// the constructor for this new object could call back into FindOrAllocate 
	// when creating its members. This can cause a deadlock so release the primary 
	// lock before constructing the new object and lock a shared mutex just for 
	// that entry.
	{
		lock_guard<recursive_mutex> lock(Mutex);
		auto it = Pointers.find(h);
		if (it == Pointers.end())
		{
			entry& e = Pointers[h];
			*_pPointer = e.pointer = allocate(_Size);
			e.init = std::this_thread::get_id();
			e.name = _Name;
			e.scope = _Scope;
			e.tracking = _Tracking;
			e.num_stack_entries = debugger::callstack(e.stack, 3);
			e.dtor_ordinal = DtorOrdinal++;
			e.dtor = _Destructor;
			reference();

			Allocated = true;
			if (_PlacementConstructor)
			{
				CallCtor = true;
				AcquireSRWLockExclusive(&e.mutex);
			}
		}
		else
		{
			AcquireSRWLockShared(&it->second.mutex); // may exist but not be constructed yet
			*_pPointer = it->second.pointer;
			ReleaseSRWLockShared(&it->second.mutex);
		}
	}

	if (CallCtor)
	{
		// Entry is already locked
		_PlacementConstructor(*_pPointer);

		// unlock the entry - how else can I do this!? I can't keep a pointer to the
		// entry because it can be moved around, so I need to safely reevaluate it.
		lock_guard<recursive_mutex> lock(Mutex);
		auto it = Pointers.find(h);
		ReleaseSRWLockExclusive(&it->second.mutex);
	}
	
	return Allocated;
}

bool context::find(const char* _Name, scope _Scope, void** _pPointer)
{
	if (!_Name || !_pPointer)
		throw std::invalid_argument("invalid argument");
	*_pPointer = nullptr;
	size_t h = hash(_Name, _Scope);
	lock_guard<recursive_mutex> lock(Mutex);
	auto it = Pointers.find(h);
	if (it != Pointers.end())
		*_pPointer = it->second.pointer;
	return !!*_pPointer;
}

void context::report()
{
	// freeing of singletons is done with atexit(). So ignore leaks that were 
	// created on this thread because they will potentially be freed after this
	// report. The traces for singleton lifetimes should indicate thread id of 
	// freeing so if there are any after this report that don't match the thread
	// if of this report, that would be bad.

	// do a pre-scan to see if it's worth printing anything to the log

	unsigned int nLeaks = 0;
	unsigned int nIgnoredLeaks = 0;
	for (container_t::const_iterator it = Pointers.begin(); it != Pointers.end(); ++it)
	{
		if (it->second.tracking == process_heap::leak_tracked)
			nLeaks++;
		else
			nIgnoredLeaks++;
	}

	path_t moduleName = this_module::get_path();
	
	char buf[2048];
	
	if (nLeaks)
	{
		char exec[128];
		snprintf(buf, "========== Process Heap Leak Report %s (Module %s) ==========\n", system::exec_path(exec), moduleName.c_str());
		debugger::print(buf);
		for (container_t::const_iterator it = Pointers.begin(); it != Pointers.end(); ++it)
		{
			if (it->second.tracking == process_heap::leak_tracked)
			{
				const entry& e = it->second;

				char TLBuf[128];
				if (e.scope == process_heap::per_thread)
				{
					snprintf(TLBuf, " (thread_local in thread 0x%x%s)"
						, asdword(e.init)
						, this_process::get_main_thread_id() == e.init ? " (main)" : "");
				}

				snprintf(buf, "%s%s\n", e.name.c_str(), e.scope == process_heap::per_thread ? TLBuf : "");
				debugger::print(buf);

				bool IsStdBind = false;
				for (size_t i = 0; i < e.num_stack_entries; i++)
				{
					bool WasStdBind = IsStdBind;
					debugger::format(buf, e.stack[i], "  ", &IsStdBind);
					if (!WasStdBind && IsStdBind) // skip a number of the internal wrappers
						i += std_bind_internal_offset;
					debugger::print(buf);
				}
			}
		}
	}

	report_footer(nLeaks);

	// For ignored leaks, release those refs on the process heap
	for (unsigned int i = 0; i < nIgnoredLeaks; i++)
		release();
}

context& context::singleton()
{
	static context* sInstance = nullptr;
	if (!sInstance)
	{
		static const guid_t GUID_ProcessHeap = { 0x7c5be6d1, 0xc5c2, 0x470e, { 0x85, 0x4a, 0x2b, 0x98, 0x48, 0xf8, 0x8b, 0xa9 } }; // {7C5BE6D1-C5C2-470e-854A-2B9848F88BA9}

		// Filename is "<GUID><CurrentProcessID>"
		path_string MappedFilename;
		to_string(MappedFilename, GUID_ProcessHeap);
		sncatf(MappedFilename, ".%u", GetCurrentProcessId());

		// Create a memory-mapped File to store the instance location so it's 
		// accessible by all code modules (EXE, DLLs)
		SetLastError(ERROR_SUCCESS);
		HANDLE hMappedFile = CreateFileMapping(INVALID_HANDLE_VALUE, 0
			, PAGE_READWRITE, 0, sizeof(context::mapped_file), MappedFilename);

		if (!hMappedFile)
			throw std::system_error(std::errc::io_error, std::system_category(), std::string("could not create process_heap memory mapped file ") + MappedFilename.c_str());
		context::mapped_file* file = 
			(context::mapped_file*)MapViewOfFile(hMappedFile, FILE_MAP_WRITE, 0, 0, 0);

		if (hMappedFile && GetLastError() == ERROR_ALREADY_EXISTS)
		{
			// File already exists, loop until it has been initialized by the 
			// creating thread.
			while (file->pid != this_process::get_id() || file->guid != GUID_ProcessHeap)
			{
				UnmapViewOfFile(file);
				::Sleep(0);
				file = (context::mapped_file*)MapViewOfFile(hMappedFile, FILE_MAP_WRITE, 0, 0, 0);
			}
			sInstance = file->instance;
		}

		else if (hMappedFile) // this is the creating thread
		{
			// Allocate memory at the highest possible address then store that value 
			// in the memory mapped file for other DLLs to access.
			*(&sInstance) = static_cast<context*>(
				VirtualAllocEx(GetCurrentProcess(), 0, sizeof(context)
				, MEM_COMMIT|MEM_RESERVE|MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE));
			
			if (!sInstance)
				throw std::system_error(std::errc::no_buffer_space, std::system_category(), "process_heap VirtualAllocEx failed");

			new (sInstance) context();

			file->instance = sInstance;
			file->guid = GUID_ProcessHeap;
			file->pid = this_process::get_id();
		}

		UnmapViewOfFile(file);
	}

	return *sInstance;
}

void ensure_initialized()
{
	context::singleton();
}

void* allocate(size_t _Size, size_t _Align)
{
	return context::singleton().allocate(_Size, _Align);
}

void deallocate(void* _Pointer)
{
	context::singleton().deallocate(_Pointer);
}

void exit_thread()
{
	context::singleton().exit_thread();
}

bool find_or_allocate(size_t _Size
	, const char* _Name
	, scope _Scope
	, tracking _Tracking
	, const std::function<void(void* _Pointer)>& _PlacementConstructor
	, const std::function<void(void* _Pointer)>& _Destructor
	, void** _pPointer)
{
	return context::singleton().find_or_allocate(_Size, _Name, _Scope, _Tracking
		, _PlacementConstructor, _Destructor, _pPointer);
}

bool find(const char* _Name, scope _Scope, void** _pPointer)
{
	return context::singleton().find(_Name, _Scope, _pPointer);
}

	} // namespace process_heap
}
