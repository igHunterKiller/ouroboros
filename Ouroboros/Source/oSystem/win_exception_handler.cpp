// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oSystem/windows/win_exception_handler.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/debugger.h>
#include <oSystem/module.h>
#include <oSystem/process.h>
#include <oSystem/process_heap.h>
#include <oSystem/system.h>
#include <oString/string.h>

#include <new.h>
#include <signal.h>
#include <oBase/type_info.h>

#undef interface
#undef INTERFACE_DEFINED

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <comdef.h>

// Additional exception. Some here are user-defined to use with RaiseException
// from a condition handler to redirect everything to one path.
#define oEXCEPTION_CPP 0xe06d7363
#define oEXCEPTION_DLL_NOT_FOUND 0xc0000135
#define oEXCEPTION_DLL_BAD_INIT 0xc0000142
#define oEXCEPTION_MODULE_NOT_FOUND 0xc06d007e
#define oEXCEPTION_PROCEDURE_NOT_FOUND 0xc06d007f
#define oEXCEPTION_PURE_VIRTUAL_CALL 0xc0de0000
#define oEXCEPTION_UNEXPECTED 0xc0de0001
#define oEXCEPTION_NEW 0xc0de0002
#define oEXCEPTION_INVALID_PARAMETER 0xc0de0003
#define oEXCEPTION_SIGABRT 0xc0de0004
#define oEXCEPTION_SIGINT 0xc0de0005
#define oEXCEPTION_SIGTERM 0xc0de0006

namespace ouro {

template<> const char* as_string(const windows::exception::type& type)
{
	static const char* s_names[] =
	{
		"unknown exception",
		"std::exception",
		"_com_error",
		"ATL::CAtlException",
	};
	return as_string(type, s_names);
}

namespace windows { namespace exception {

const char* as_string_exception_code(int ex_code)
{
	switch (ex_code)
	{
		case EXCEPTION_ACCESS_VIOLATION: return "EXCEPTION_ACCESS_VIOLATION";
		case EXCEPTION_DATATYPE_MISALIGNMENT: return "EXCEPTION_DATATYPE_MISALIGNMENT";
		case EXCEPTION_BREAKPOINT: return "EXCEPTION_BREAKPOINT";
		case EXCEPTION_SINGLE_STEP: return "EXCEPTION_SINGLE_STEP";
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
		case EXCEPTION_FLT_DENORMAL_OPERAND: return "EXCEPTION_FLT_DENORMAL_OPERAND";
		case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
		case EXCEPTION_FLT_INEXACT_RESULT: return "EXCEPTION_FLT_INEXACT_RESULT";
		case EXCEPTION_FLT_INVALID_OPERATION: return "EXCEPTION_FLT_INVALID_OPERATION";
		case EXCEPTION_FLT_OVERFLOW: return "EXCEPTION_FLT_OVERFLOW";
		case EXCEPTION_FLT_STACK_CHECK: return "EXCEPTION_FLT_STACK_CHECK";
		case EXCEPTION_FLT_UNDERFLOW: return "EXCEPTION_FLT_UNDERFLOW";
		case EXCEPTION_INT_DIVIDE_BY_ZERO: return "EXCEPTION_INT_DIVIDE_BY_ZERO";
		case EXCEPTION_INT_OVERFLOW: return "EXCEPTION_INT_OVERFLOW";
		case EXCEPTION_PRIV_INSTRUCTION: return "EXCEPTION_PRIV_INSTRUCTION";
		case EXCEPTION_IN_PAGE_ERROR: return "EXCEPTION_IN_PAGE_ERROR";
		case EXCEPTION_ILLEGAL_INSTRUCTION: return "EXCEPTION_ILLEGAL_INSTRUCTION";
		case EXCEPTION_NONCONTINUABLE_EXCEPTION: return "EXCEPTION_NONCONTINUABLE_EXCEPTION";
		case EXCEPTION_STACK_OVERFLOW: return "EXCEPTION_STACK_OVERFLOW";
		case EXCEPTION_INVALID_DISPOSITION: return "EXCEPTION_INVALID_DISPOSITION";
		case EXCEPTION_GUARD_PAGE: return "EXCEPTION_GUARD_PAGE";
		case EXCEPTION_INVALID_HANDLE: return "EXCEPTION_INVALID_HANDLE";
		//case EXCEPTION_POSSIBLE_DEADLOCK: return "EXCEPTION_POSSIBLE_DEADLOCK";
		case oEXCEPTION_CPP: return "oEXCEPTION_CPP'";
		case oEXCEPTION_DLL_NOT_FOUND: return "oEXCEPTION_DLL_NOT_FOUND'";
		case oEXCEPTION_DLL_BAD_INIT: return "oEXCEPTION_DLL_BAD_INIT'";
		case oEXCEPTION_MODULE_NOT_FOUND: return "oEXCEPTION_MODULE_NOT_FOUND'";
		case oEXCEPTION_PROCEDURE_NOT_FOUND: return "oEXCEPTION_PROCEDURE_NOT_FOUND'";
		case oEXCEPTION_PURE_VIRTUAL_CALL: return "oEXCEPTION_PURE_VIRTUAL_CALL'";
		case oEXCEPTION_UNEXPECTED: return "oEXCEPTION_UNEXPECTED'";
		case oEXCEPTION_NEW: return "oEXCEPTION_NEW'";
		case oEXCEPTION_INVALID_PARAMETER: return "oEXCEPTION_INVALID_PARAMETER'";
		case oEXCEPTION_SIGABRT: return "oEXCEPTION_SIGABRT'";
		case oEXCEPTION_SIGINT: return "oEXCEPTION_SIGINT'";
		case oEXCEPTION_SIGTERM: return "oEXCEPTION_SIGTERM'";
		default: break;
	}

	return "unrecognized Exception Code";
}

static void pure_virtual_call_handler()
{
	RaiseException(oEXCEPTION_PURE_VIRTUAL_CALL, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}

static int new_handler(size_t size)
{
	ULONG_PTR sz = (ULONG_PTR)size;
	RaiseException(oEXCEPTION_NEW, EXCEPTION_NONCONTINUABLE, 1, &sz);
	return 0;
}

static void invalid_parameter_handler(const wchar_t* expression, const wchar_t* function, const wchar_t* file, unsigned int line, uintptr_t reserved)
{
	ULONG_PTR params[5] = { (ULONG_PTR)expression, (ULONG_PTR)function, (ULONG_PTR)file, (ULONG_PTR)line, (ULONG_PTR)reserved };
	RaiseException(oEXCEPTION_NEW, EXCEPTION_NONCONTINUABLE, 5, params);
}

static void sigabrt_handler(int sig)
{
	ULONG_PTR v = (ULONG_PTR)sig;
	//RaiseException(oEXCEPTION_SIGABRT, EXCEPTION_NONCONTINUABLE, 1, &v);
}

static void sigint_handler(int sig)
{
	ULONG_PTR v = (ULONG_PTR)sig;
	RaiseException(oEXCEPTION_SIGINT, EXCEPTION_NONCONTINUABLE, 1, &v);
}

static void sigterm_handler(int sig)
{
	ULONG_PTR v = (ULONG_PTR)sig;
	RaiseException(oEXCEPTION_SIGTERM, EXCEPTION_NONCONTINUABLE, 1, &v);
}

static void unexpected_handler()
{
	RaiseException(oEXCEPTION_UNEXPECTED, EXCEPTION_NONCONTINUABLE, 0, nullptr);
}

static void redirect_handlers_to_exception()
{
	_set_purecall_handler(pure_virtual_call_handler);
	_set_new_handler(new_handler);
	set_unexpected(unexpected_handler);
	_set_invalid_parameter_handler(invalid_parameter_handler); 
	_set_abort_behavior(_CALL_REPORTFAULT, _CALL_REPORTFAULT);
	signal(SIGABRT, sigabrt_handler);
	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigterm_handler);
}

static const ::type_info* get_veh_type_info(const EXCEPTION_RECORD& record)
{
	// http://blogs.msdn.com/b/oldnewthing/archive/2010/07/30/10044061.aspx
	if (record.ExceptionCode != oEXCEPTION_CPP)
		return nullptr;
	
	#ifdef _WIN64
		HMODULE hModule = (HMODULE)record.ExceptionInformation[3];
	#else
		HMODULE hModule = nullptr;
	#endif
	if (!hModule)
		return nullptr;

	DWORD a = ((DWORD*)record.ExceptionInformation[2])[3];
	DWORD b = ((DWORD*)  ((char*)hModule + a))[1];
	DWORD c = ((DWORD*)  ((char*)hModule + b))[1];
	return (::type_info*)((char*)hModule + c);
}

static cpp_exception get_veh_exception(const EXCEPTION_RECORD& record)
{
	cpp_exception e;
	if (record.ExceptionCode == oEXCEPTION_CPP)
	{
		e.void_exception = (void*)record.ExceptionInformation[1];
		const ::type_info* ti = get_veh_type_info(record);
		if (ti)
		{
			e.type_name = type_name(ti->name());
			
			// how can this deal with exception derived from std::exception?
			// Weak answer for now: assume it's either namespaced in std:: or
			// it is postfixed with _exception.
			
			if (strstr(e.type_name, "_com_error"))
			{
				e.type = exception::type::com;
				e.what = category().message(e.com_error->Error());
			}
			
			else if (strstr(e.type_name, "CAtlException"))
			{
				e.type = exception::type::atl;
			}
			
			else if (strstr(e.type_name, "std::") || strstr(e.type_name, "_exception") || strstr(e.type_name, "_error"))
			{
				e.type = exception::type::std;
				e.what = e.std_exception->what();
			}
		}
	}
	return e;
}

class context
{
public:
	static context& singleton();

	inline void set_handler(handler_fn handler, void* user) { handler_ = handler; user_ = user; }

	// Specify a directory where mini/full dumps will be written. Dump filenames 
	// will be generated with a timestamp at the time of exception.
	inline void mini_dump_path(const path_t& mini_dump_path) { minidump_ = mini_dump_path; }
	inline const path_t& mini_dump_path() const { return minidump_; }

	inline void full_dump_path(const path_t& full_dump_path) { fulldump_ = full_dump_path; }
	inline const path_t& full_dump_path() const { return fulldump_; }

	inline void post_dump_command(const char* command) { post_dump_command_ = command; }
	inline const char* post_dump_command() const { return post_dump_command_; }

	inline void prompt_after_dump(bool prompt) { prompt_after_dump_ = prompt; }
	inline bool prompt_after_dump() const { return prompt_after_dump_; }
	
private:
	context();
	~context();

	static const ::type_info* get_type_info(const EXCEPTION_RECORD& record);
	static const void* get_exception(const EXCEPTION_RECORD& record);
	static LONG WINAPI static_on_exception(EXCEPTION_POINTERS* exception_pointers);
	LONG on_exception(EXCEPTION_POINTERS* exception_pointers);

	handler_fn handler_;
	void*      user_;
	path_t     minidump_;
	path_t     fulldump_;
	xlstring   post_dump_command_;
	bool       prompt_after_dump_;
};

// Allows us to break execution when an access violation occurs
LONG context::on_exception(EXCEPTION_POINTERS* exception_pointers)
{
	EXCEPTION_RECORD* pRecord = exception_pointers->ExceptionRecord;
	cpp_exception cpp_exc;
	switch (pRecord->ExceptionCode)
	{
		case EXCEPTION_ACCESS_VIOLATION:
		{
			void* addr      = (void*)pRecord->ExceptionInformation[1];
			const char* err = (0 ==  pRecord->ExceptionInformation[0]) ? "Read" : "Write";
			char msg[512];
			snprintf(msg, "%s access violation at 0x%p", err, addr);
			if (handler_)
				handler_(msg, cpp_exc, (uintptr_t)exception_pointers, user_);
			break;
		}

		// This handler occurs before C++ gets its chance to catch an exception, so
		// don't get in the way of that.
		case oEXCEPTION_CPP:
		{
			cpp_exc = get_veh_exception(*pRecord);
			if (!cpp_exc.what.empty())
			{
				char msg[2048];
				path_t module_path = this_module::get_path();

				#ifdef _WIN64
					#define LOWER_CASE_PTR_FMT "%016llx"
				#else
					#define LOWER_CASE_PTR_FMT "%08x"
				#endif

				snprintf(msg, "First-chance exception at 0x" LOWER_CASE_PTR_FMT ": in %s: %s: %s\n"
					, pRecord->ExceptionAddress, module_path.filename().c_str(), cpp_exc.type_name, cpp_exc.what.c_str());
				OutputDebugStringA(msg);
			}
			break;
		}

		// Ensure any exception that is to be handled is listed explicitly. There
		// are some working-as-intended uses of exception by Windows that should 
		// not terminate execution, but instead should be passed through.
		case EXCEPTION_DATATYPE_MISALIGNMENT:
		case EXCEPTION_BREAKPOINT:
		case EXCEPTION_SINGLE_STEP:
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
		case EXCEPTION_FLT_DENORMAL_OPERAND:
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_INEXACT_RESULT:
		case EXCEPTION_FLT_INVALID_OPERATION:
		case EXCEPTION_FLT_OVERFLOW:
		case EXCEPTION_FLT_STACK_CHECK:
		case EXCEPTION_FLT_UNDERFLOW:
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_INT_OVERFLOW:
		case EXCEPTION_PRIV_INSTRUCTION:
		case EXCEPTION_IN_PAGE_ERROR:
		case EXCEPTION_ILLEGAL_INSTRUCTION:
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
		case EXCEPTION_STACK_OVERFLOW:
		case EXCEPTION_INVALID_DISPOSITION:
		case EXCEPTION_GUARD_PAGE:
		case EXCEPTION_INVALID_HANDLE:
		case oEXCEPTION_PURE_VIRTUAL_CALL:
		case oEXCEPTION_UNEXPECTED:
		case oEXCEPTION_DLL_NOT_FOUND:
		case oEXCEPTION_DLL_BAD_INIT:
		case oEXCEPTION_MODULE_NOT_FOUND:
		case oEXCEPTION_PROCEDURE_NOT_FOUND:
		case oEXCEPTION_SIGINT:
		case oEXCEPTION_SIGTERM:
			if (handler_)
				handler_(as_string_exception_code(pRecord->ExceptionCode), cpp_exc, (uintptr_t)exception_pointers, user_);
			break;

		default:
			break;
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

LONG context::static_on_exception(EXCEPTION_POINTERS* exception_pointers)
{
	return context::singleton().on_exception(exception_pointers);
}

context::context()
	: handler_(nullptr)
	, user_(nullptr)
	, prompt_after_dump_(false)
{
	AddVectoredExceptionHandler(0, static_on_exception);
	redirect_handlers_to_exception();
}

context::~context()
{
	RemoveVectoredExceptionHandler(static_on_exception);
}

oDEFINE_PROCESS_SINGLETON("windows::exception", context);

void set_handler(handler_fn handler, void* user)
{
	context::singleton().set_handler(handler, user);
}

void mini_dump_path(const path_t& mini_dump_path)
{
	context::singleton().mini_dump_path(mini_dump_path);
}

const path_t& mini_dump_path()
{
	return context::singleton().mini_dump_path();
}

void full_dump_path(const path_t& full_dump_path)
{
	context::singleton().full_dump_path(full_dump_path);
}

const path_t& full_dump_path()
{
	return context::singleton().full_dump_path();
}

void post_dump_command(const char* command)
{
	context::singleton().post_dump_command(command);
}

const char* post_dump_command()
{
	return context::singleton().post_dump_command();
}

void prompt_after_dump(bool prompt)
{
	context::singleton().prompt_after_dump(prompt);
}

bool prompt_after_dump()
{
	return context::singleton().prompt_after_dump();
}

void enable_dialogs(bool enable)
{
	if (enable)
	{
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_WNDW);
	}

	else
	{
		_CrtSetReportFile(_CRT_ASSERT, stderr);
		_CrtSetReportFile(_CRT_ERROR, stderr);
		_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
		_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	}
}

static void handle_exception(const char* message, const cpp_exception& cpp_exception, uintptr_t exception_context, void* user)
{
	if (!this_process::has_debugger_attached())
	{
		#ifdef _DEBUG
			oAssertTrace(assertion, abort, "", "%s", message);
			oAssert(false, "%s", message);
		#else
			debugger::dump_and_terminate((void*)exception_context, message);
		#endif
	}
};

struct install_exception_handler
{
	install_exception_handler()
	{
		process_heap::ensure_initialized(); // ensure process_heap is around to track the handler
		windows::exception::set_handler(handle_exception, nullptr);
	}
};

static install_exception_handler s_install_exception_handler;

}}}
