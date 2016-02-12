// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oSystem/debugger.h>
#include <oSystem/filesystem.h>
#include <oSystem/module.h>
#include <oSystem/process_heap.h>
#include <oSystem/system.h>
#include <oSystem/windows/win_util.h>
#include <oSystem/windows/win_exception_handler.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <DbgHelp.h>

using namespace std;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

namespace ouro { namespace debugger {

void thread_name(const char* name, std::thread::id id)
{
	if (name && *name && IsDebuggerPresent())
	{
		// http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx
		Sleep(10);
		THREADNAME_INFO i;
		i.dwType = 0x1000;
		i.szName = name;
		i.dwThreadID = id == std::thread::id() ? GetCurrentThreadId() : asdword(id);
		i.dwFlags = 0;
		const static DWORD MS_VCexception = 0x406D1388;
		__try { RaiseException(MS_VCexception, 0, sizeof(i)/sizeof(ULONG_PTR), (ULONG_PTR*)&i); }
		__except(EXCEPTION_EXECUTE_HANDLER) {}
	}
}

void print(const char* string)
{
	OutputDebugStringA(string);
}

void break_on_alloc(uintptr_t alloc_id)
{
	_CrtSetBreakAlloc((long)alloc_id);
}

// Old StackWalk64. CaptureStackBackTrace is simpler and quicker.
#if 0
// http://jpassing.wordpress.com/category/win32/
// Jochen Kalmback (BSD)

// Several of the API assume use of CHAR, not TCHAR
static_assert(sizeof(CHAR) == sizeof(char), "size mismatch");

static void init_context(CONTEXT& c)
{
	memset(&c, 0, sizeof(c));
	RtlCaptureContext(&c);
}

static void init_stack_frame(STACKFRAME64& s, const CONTEXT& c)
{
  memset(&s, 0, sizeof(s));
  s.AddrPC.Mode = AddrModeFlat;
  s.AddrFrame.Mode = AddrModeFlat;
  s.AddrStack.Mode = AddrModeFlat;

	#ifdef _M_IX86
	  s.AddrPC.Offset = c.Eip;
	  s.AddrFrame.Offset = c.Ebp;
	  s.AddrStack.Offset = c.Esp;
	#elif _M_X64
	  s.AddrPC.Offset = c.Rip;
	  s.AddrFrame.Offset = c.Rsp;
	  s.AddrStack.Offset = c.Rsp;
	#elif _M_IA64
	  s.AddrPC.Offset = c.StIIP;
	  s.AddrFrame.Offset = c.IntSp;
	  s.AddrStack.Offset = c.IntSp;
	  s.AddrBStore.Offset = c.RsBSP;
	  s.AddrBStore.Mode = AddrModeFlat;
	#else
		#error "Platform not supported!"
	#endif
}

static DWORD imagetype()
{
	#ifdef _M_IX86
		return IMAGE_FILE_MACHINE_I386;
	#elif _M_X64
		return IMAGE_FILE_MACHINE_AMD64;
	#elif _M_IA64
		return IMAGE_FILE_MACHINE_IA64;
	#else
		#error Unsupported platform
	#endif
}

size_t callstack(symbol_t* out_symbols, size_t num_symbols, size_t offset)
{
	CONTEXT c;
	init_context(c);

	STACKFRAME64 s;
	init_stack_frame(s, c);

	size_t n = 0;
	while (n < num_symbols)
	{
		if (!StackWalk64(imagetype(), GetCurrentProcess(), GetCurrentThread(), &s, &c, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0))
			break;

		if (s.AddrReturn.Offset == 0 || s.AddrReturn.Offset == 0xffffffffcccccccc)
			break;

		if (offset)
		{
			offset--;
			continue;
		}

		_pSymbols[n++] = s.AddrReturn.Offset;
	}

	return n;
}
#else
size_t callstack(symbol_t* out_symbols, size_t num_symbols, size_t offset)
{
	// + 1 skips this call to callstack
	return CaptureStackBackTrace(static_cast<ULONG>(offset + 1), static_cast<ULONG>(num_symbols), (PVOID*)out_symbols, nullptr);
}
#endif

bool sdkpath(char* path, size_t path_size, const char* sdk_relative_path)
{
	bool result = false;
	DWORD len = GetEnvironmentVariableA("ProgramFiles", path, static_cast<DWORD>(path_size));
	if (len && len < path_size)
	{
		strlcat(path, sdk_relative_path, path_size);
		result = GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
	}

	return result;
}

template<size_t size> inline BOOL sdkpath(char (&path)[size], const char* sdk_relative_path) { return sdkpath(path, size, sdk_relative_path); }

static BOOL CALLBACK load_module(PCSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext)
{
	//const oWinDbgHelp* d = static_cast<const oWinDbgHelp*>(UserContext);
	HANDLE hProcess = (HANDLE)UserContext;
	bool success = !!SymLoadModule64(hProcess, 0, ModuleName, 0, ModuleBase, ModuleSize);
	//if (d->GetModuleLoadedHandler())
	//	(*d->GetModuleLoadedHandler())(ModuleName, success);
	return success;
}

static HMODULE init_dbghelp(HANDLE hprocess
	, bool softlink
	, const char* symbol_path
	, char* outsymbol_search_path
	, size_t symbol_search_path_size)
{
	HMODULE hDbgHelp = nullptr;

	if (!hprocess || hprocess == INVALID_HANDLE_VALUE)
		hprocess = GetCurrentProcess();

	char path[512];
	if (softlink)
	{
		GetModuleFileNameA(0, path, countof(path));
		if (!GetLastError())
		{
			// first local override
			strlcat(path, ".local");
			if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES)
			{
				// then for an installed version (32/64-bit)
				if (sdkpath(path, "/Debugging Tools for Windows/dbghelp.dll") && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
					hDbgHelp = LoadLibraryA(path);

				if (!hDbgHelp && sdkpath(path, "/Debugging Tools for Windows 64-Bit/dbghelp.dll") && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
					hDbgHelp = LoadLibraryA(path);
			}
		}

		// else punt to wherever the system can find it
		if (!hDbgHelp)
			hDbgHelp = LoadLibraryA("dbghelp.dll");
	}

	if (hDbgHelp || !softlink)
	{
		if (outsymbol_search_path)
			*outsymbol_search_path = '\0';

		// Our PDBs no longer have absolute paths, so we set the search path to
		// the executable path, unless the user specified symbol_path
		if (!symbol_path || !*symbol_path)
		{
			GetModuleFileNameA(0, path, countof(path));
			*(rstrstr(path, "\\") + 1) = '\0'; // trim filename
		}
		else
			strlcpy(path, symbol_path);

		if (SymInitialize(hprocess, path, FALSE))
		{
			SymSetOptions(SymGetOptions() | SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS);
			if (outsymbol_search_path)
				SymGetSearchPath(hprocess, outsymbol_search_path, static_cast<DWORD>(symbol_search_path_size));

			EnumerateLoadedModules64(hprocess, &load_module, hprocess);
		}
		else
			throw std::system_error(std::errc::io_error, std::system_category());
	}

	return hDbgHelp;
}

symbol_info translate(symbol_t symbol)
{
	#ifdef _WIN64
		#define oIMAGEHLP_MODULE IMAGEHLP_MODULE64
		#define oIMAGEHLP_SYMBOL IMAGEHLP_SYMBOL64
		#define oIMAGEHLP_LINE IMAGEHLP_LINE64
		#define oSymGetModuleInfo SymGetModuleInfo64
		#define oSymGetSymFromAddr SymGetSymFromAddr64
		#define oSymGetLineFromAddr SymGetLineFromAddr64
		#define oDWORD DWORD64
	#else
		#define oIMAGEHLP_MODULE IMAGEHLP_MODULE
		#define oIMAGEHLP_SYMBOL IMAGEHLP_SYMBOL
		#define oIMAGEHLP_LINE IMAGEHLP_LINE
		#define oSymGetModuleInfo SymGetModuleInfo
		#define oSymGetSymFromAddr SymGetSymFromAddr
		#define oSymGetLineFromAddr SymGetLineFromAddr
		#define oDWORD DWORD
	#endif

	symbol_info si;
	si.address = symbol;

	if (!si.address)
	{
		si.module = "(null)";
		si.filename = "(null)";
		si.name = "(null)";
		si.symboloffset = 0;
		si.line = 0;
		si.charoffset = 0;
		return si;
	}

	oIMAGEHLP_MODULE module;
	memset(&module, 0, sizeof(module));
	module.SizeOfStruct = sizeof(module);

	if (!oSymGetModuleInfo(GetCurrentProcess(), symbol, &module))
	{
		init_dbghelp(nullptr, false, nullptr, nullptr, 0);
		oVB(oSymGetModuleInfo(GetCurrentProcess(), symbol, &module));
	}
	
	si.module = module.ModuleName;
	BYTE buf[sizeof(oIMAGEHLP_SYMBOL) + mstring::Capacity * sizeof(TCHAR)];
	oIMAGEHLP_SYMBOL* symbolInfo = (oIMAGEHLP_SYMBOL*)buf;
	memset(buf, 0, sizeof(oIMAGEHLP_SYMBOL));
	symbolInfo->SizeOfStruct = sizeof(oIMAGEHLP_SYMBOL);
	symbolInfo->MaxNameLength = static_cast<DWORD>(si.name.capacity());

	oDWORD displacement = 0;
	oVB(oSymGetSymFromAddr(GetCurrentProcess(), symbol, &displacement, symbolInfo));

	// symbolInfo just contains the first 512 characters and doesn't guarantee
	// they will be null-terminated, so copy the buffer and ensure there's some
	// rational terminator
	//strcpy(si.name, symbolInfo->Name);
	memcpy(si.name, symbolInfo->Name, si.name.capacity() - sizeof(TCHAR));
	ellipsize(si.name);
	si.symboloffset = static_cast<unsigned int>(displacement);

 	oIMAGEHLP_LINE line;
	memset(&line, 0, sizeof(line));
	line.SizeOfStruct = sizeof(line);

	DWORD disp = 0;
	if (oSymGetLineFromAddr(GetCurrentProcess(), symbol, &disp, &line))
	{
		si.filename = line.FileName;
		si.line = line.LineNumber;
		si.charoffset = disp;
	}

	else
	{
		si.filename.clear();
		si.line = 0;
		si.charoffset = 0;
	}

	return si;
}

static bool is_std_bind_impl_detail(const char* symbol)
{
	static struct { const char* bind_str; size_t len; } s_bindstrs[] = 
	{
		{ "std::tr1::_Pmf", 14 },
		{ "std::tr1::_Callable_", 20 },
		{ "std::tr1::_Bind", 15 },
		{ "std::tr1::_Impl", 15 },
		{ "std::tr1::_Function", 19 },
	};

	for (const auto& s : s_bindstrs)
		if (!memcmp(symbol, s.bind_str, s.len))
			return true;
	return false;
}

int format(char* dst, size_t dst_size, symbol_t symbol, const char* prefix, bool* inout_is_std_bind)
{
	int rv = 0;
	symbol_info s = translate(symbol);
	if (inout_is_std_bind && is_std_bind_impl_detail(s.name))
	{
		if (!*inout_is_std_bind)
		{
			rv = snprintf(dst, dst_size, "%s... std::bind ...\n", prefix);
			*inout_is_std_bind = true;
		}
	}

	else
	{
		if (inout_is_std_bind)
			*inout_is_std_bind = false;

		path_t fn_ = s.filename.filename();
		const char* fn = fn_.empty() ? "unknown" : fn_.c_str();

		if (s.line && s.charoffset)
			rv = snprintf(dst, dst_size, "%s%s!%s() ./%s Line %i + 0x%0x bytes\n", prefix, s.module.c_str(), s.name.c_str(), fn, s.line, s.charoffset);
		else if (s.line)
			rv = snprintf(dst, dst_size, "%s%s!%s() ./%s Line %i\n", prefix, s.module.c_str(), s.name.c_str(), fn, s.line);
		else
			rv = snprintf(dst, dst_size, "%s%s!%s() ./%s\n", prefix, s.module.c_str(), s.name.c_str(), fn);
	}

	return rv;
}

void print_callstack(char* dst, size_t dst_size, size_t offset, bool filter_std_bind)
{
	int res = 0;
	size_t len = 0;

	size_t nSymbols = 0;
	*dst = 0;
	symbol_t address = 0;
	bool IsStdBind = false;
	while (callstack(&address, 1, offset++))
	{
		if (nSymbols++ == 0) // if we have a callstack, label it
		{
			res = _snprintf_s(dst, dst_size, _TRUNCATE, "\nCall Stack:\n");
			if (res == -1) goto TRUNCATION;
			len += res;
		}

		bool WasStdBind = IsStdBind;
		res = format(&dst[len], dst_size - len - 1, address, "", &IsStdBind);
		if (res == -1) goto TRUNCATION;
		len += res;

		if (!WasStdBind && IsStdBind) // skip a number of the internal wrappers
			offset += 5;
	}

	return;

TRUNCATION:
	static const char* kStackTooLargeMessage = "\n... truncated ...";
	size_t TLMLength = strlen(kStackTooLargeMessage);
	snprintf(dst + dst_size - 1 - TLMLength, TLMLength + 1, kStackTooLargeMessage);
}

static int write_dump_file(MINIDUMP_TYPE type, HANDLE hfile, bool* out_success, EXCEPTION_POINTERS* outexception_pointers)
{
	_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	ExInfo.ThreadId = GetCurrentThreadId();
	ExInfo.ExceptionPointers = outexception_pointers;
	ExInfo.ClientPointers = TRUE; // true because we're in THIS process, this might need to change if this is called from another process.
	*out_success = !!MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hfile, type, outexception_pointers ? &ExInfo : nullptr, nullptr, nullptr);
	return EXCEPTION_EXECUTE_HANDLER;
}

bool dump(const path_t& path, bool full, void* exceptions)
{
	// Use most-direct APIs for this so there's less chance another crash/assert
	// can occur.

	filesystem::create_directories(path.parent_path());
	HANDLE hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	const MINIDUMP_TYPE kType = full ? MiniDumpWithFullMemory : MiniDumpNormal;
	bool success = false; 
	if (exceptions)
		write_dump_file(kType, hFile, &success, (EXCEPTION_POINTERS*)exceptions);
	else
	{
		// If you're here, especially from a dump file, it's because the file was 
		// dumped outside an exception handler. In order to get stack info, we need
		// to cause an exception. See Remarks section:
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms680360(v=vs.85).aspx

		// So from here, somewhere up the stack should be the line of code that 
		// triggered execution of this
		const static DWORD FORCEexception_FOR_CALLSTACK_INFO = 0x1337c0de;
		__try { RaiseException(FORCEexception_FOR_CALLSTACK_INFO, 0, 0, nullptr); }
		__except(write_dump_file(kType, hFile, &success, GetExceptionInformation())) {}
	}

	CloseHandle(hFile);
	return success;
}

static sstring make_dump_filename()
{
	sstring DumpFilename = this_module::get_path().basename();
	DumpFilename += "_v";
	
	module::info mi = this_module::get_info();
	to_string(&DumpFilename[1], DumpFilename.capacity() - 1, mi.version);
	DumpFilename += "D";

	ntp_date now;
	system::now(&now);
	sstring StrNow;
	strftime(DumpFilename.c_str() + DumpFilename.length()
		, DumpFilename.capacity() - DumpFilename.length()
		, syslog_local_date_format
		, now
		, date_conversion::to_local);

	replace(StrNow, DumpFilename, ":", "_");
	replace(DumpFilename, StrNow, ".", "_");

	return DumpFilename;
}

void dump_and_terminate(void* exception, const char* message)
{
	sstring DumpFilename = make_dump_filename();

	bool Mini = false;
	bool Full = false;

	char DumpPath[oMAX_PATH];
	const path_t& MiniDumpPath = windows::exception::mini_dump_path();
	if (!MiniDumpPath.empty())
	{
		snprintf(DumpPath, "%s%s.dmp", MiniDumpPath.c_str(), DumpFilename.c_str());
		Mini = dump(path_t(DumpPath), false, exception);
	}

	const path_t& FullDumpPath = windows::exception::full_dump_path();
	if (!FullDumpPath.empty())
	{
		snprintf(DumpPath, "%s%s.dmp", FullDumpPath.c_str(), DumpFilename.c_str());
		Full = dump(path_t(DumpPath), true, exception);
	}

	const char* PostDumpCommand = windows::exception::post_dump_command();

	if (PostDumpCommand && *PostDumpCommand)
		::system(PostDumpCommand);

	if (windows::exception::prompt_after_dump())
	{
		char msg[2048];
		snprintf(msg, "%s\n\nThe program will now exit.%s"
			, message
			, Mini || Full ? "\n\nA .dmp file has been written." : "");

		MessageBox(NULL, msg, this_module::get_path().c_str(), MB_ICONERROR|MB_OK|MB_TASKMODAL); 
	}
	std::exit(-1);
}

}}
