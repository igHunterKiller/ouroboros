// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oSystem/reporting.h>
#include <oMemory/fnv1a.h>
#include <oCore/algorithm.h>
#include <oBase/fixed_vector.h>
#include <oSystem/debugger.h>
#include <oSystem/filesystem.h>
#include <oSystem/process.h>
#include <oSystem/process_heap.h>
#include <oSystem/system.h>
#include <oSystem/windows/win_exception_handler.h>
#include <mutex>

#include <oGUI/msgbox.h>

namespace ouro {

const char* as_string(const assert_type& type)
{
	switch (type)
	{
		case assert_type::trace: return "Trace";
		case assert_type::assertion: return "Error";
		default: break;
	}
	return "?";
}

	namespace reporting {

void report_and_exit()
{
	oTraceA("std::terminate called");
	oAssert(false, "std::terminate called");
}

void emit_debugger(const assert_context& ctx, const char* msg)
{
	debugger::print(msg);
}

void emit_log(const assert_context& ctx, const char* msg, filesystem::file_handle hfile)
{
	if (hfile)
		filesystem::write(hfile, msg, strlen(msg), true);
}

#define LOCK() std::lock_guard<std::recursive_mutex> lock(mtx)

class context
{
public:
	static context& singleton();

	inline void set_info(const info& i) { LOCK(); inf = i; }
	inline info get_info() const { LOCK(); return inf; }

	assert_action vformatf(const assert_context& ctx, const char* fmt, va_list args) const;

	inline int add_emitter(const emitter& e) { LOCK(); return (int)sparse_set(emitters, e); }
	inline void remove_emitter(int emitter) { LOCK(); safe_set(emitters, emitter, nullptr); }
	void set_log(const path_t& p);
	inline path_t get_log() const { return filesystem::get_path(log); }
	inline void set_prompter(const prompter& p) { LOCK(); prompter = p; }
	inline void add_filter(uint32_t id) { LOCK(); push_back_unique(filters, id); }
	inline void remove_filter(uint32_t id) { LOCK(); find_and_erase(filters, id); }

private:
	context();
	~context();

	info inf;
	mutable std::recursive_mutex mtx;

	fixed_vector<uint32_t, 256> filters;
	fixed_vector<emitter, 16> emitters;
	prompter prompter;
	filesystem::scoped_file log;
	int logemitter;
};

context::context()
	: logemitter(-1)
{
	std::set_terminate(report_and_exit);
	set_info(inf);

	add_emitter(emit_debugger);
}

context::~context()
{
	std::set_terminate(nullptr);
}

oDEFINE_PROCESS_SINGLETON("ouro::reporting", context);

// Returns a pointer to the nul terminator of dst, or beyond its 
// size if filled.
static char* formatmsg(char* dst, size_t dst_size, const assert_context& ctx, const info& info
	, char** puser_msg_start // points into dst at end of prefix info
	, const char* fmt
	, va_list args)
{
	if (puser_msg_start)
		*puser_msg_start = dst;

	#define oACCUMF(fmt, ...) do \
	{	res = snprintf(dst + len, dst_size - len - 1, fmt, ## __VA_ARGS__); \
		if (res == -1) goto TRUNCATION; \
		len += res; \
	} while(false)

	#define oVACCUMF(fmt, args) do \
	{	res = ouro::vsnprintf(dst + len, dst_size - len - 1, fmt, args); \
		if (res == -1) goto TRUNCATION; \
		len += res; \
	} while(false)

	*dst = '\0';

	int res = 0;
	size_t len = 0;
	if (info.prefix_file_line)
	{
		#if _MSC_VER < 1600
			static const char* kClickableFileLineFormat = "%s(%u) : ";
		#else
			static const char* kClickableFileLineFormat = "%s(%u): ";
		#endif
		oACCUMF(kClickableFileLineFormat, ctx.filename, ctx.line);
	}

	if (info.prefix_timestamp)
	{
		ntp_timestamp now = 0;
		system::now(&now);
		res = (int)strftime(dst + len
			, dst_size - len - 1
			, sortable_date_ms_format
			, now
			, date_conversion::to_local);
		oACCUMF(" ");
		if (res == 0) goto TRUNCATION;
		len += res;
	}

	if (info.prefix_type)
		oACCUMF("%s: ", as_string(ctx.type));

	if (info.prefix_thread_id)
	{
		mstring exec;
		oACCUMF("%s ", system::exec_path(exec));
	}

	if (info.prefix_id)
		oACCUMF("{0x%08x} ", fnv1a<uint32_t>(fmt));

	if (puser_msg_start)
		*puser_msg_start = dst + len;

	oVACCUMF(fmt, args);

	return dst + len;

TRUNCATION:
	static const char* kStackTooLargeMessage = "\n... truncated ...";
	size_t TLMLength = strlen(kStackTooLargeMessage);
	snprintf(dst + dst_size - 1 - TLMLength, TLMLength + 1, kStackTooLargeMessage);
	return dst + dst_size;

	#undef oACCUMF
	#undef oVACCUMF
}

assert_action context::vformatf(const assert_context& ctx
	, const char* fmt
	, va_list args) const
{
	assert_action action = ctx.default_response;

	{
		uint32_t ID = fnv1a<uint32_t>(fmt);
		LOCK();
		if (std::find(filters.cbegin(), filters.cend(), ID) != filters.cend())
			return action;
	}

	char msg[8192];
	char* usr = nullptr;
	char* cur = formatmsg(msg, sizeof(msg), ctx, inf, &usr, fmt, args);
	char* end = msg + sizeof(msg);

	if (inf.callstack && ctx.type == assert_type::assertion)
		debugger::print_callstack(cur, std::distance(cur, end), 6, true);

	{
		LOCK();

		for (auto& e : emitters)
			if (e) e(ctx, msg);

		if (prompter)
			action = prompter(ctx, usr);
	}

	if (ctx.type == assert_type::assertion)
	{
		oTrace("%s (%i): %s", ctx.filename, ctx.line, msg);
		if (!this_process::has_debugger_attached())
			debugger::dump_and_terminate(nullptr, nullptr);
	}

	return action;
}

void context::set_log(const path_t& p)
{
	if (logemitter >= 0)
	{
		remove_emitter(logemitter);
		logemitter = -1;
	}

	log = std::move(filesystem::scoped_file(p, filesystem::open_option::text_append));
	if (log)
		logemitter = add_emitter(std::bind(emit_log, std::placeholders::_1, std::placeholders::_2, (filesystem::file_handle)log));
}

void ensure_initialized()
{
	context::singleton();
}

void set_info(const info& i)
{
	context::singleton().set_info(i);
}

info get_info()
{
	return context::singleton().get_info();
}

void vformatf(const assert_context& ctx, const char* fmt, va_list args)
{
	context::singleton().vformatf(ctx, fmt, args);
}

int add_emitter(const emitter& e)
{
	return context::singleton().add_emitter(e);
}

void remove_emitter(int emitter)
{
	context::singleton().remove_emitter(emitter);
}

void set_log(const path_t& p)
{
	context::singleton().set_log(p);
}

path_t get_log()
{
	return context::singleton().get_log();
}

void set_prompter(const prompter& p)
{
	context::singleton().set_prompter(p);
}

void add_filter(uint32_t id)
{
	context::singleton().add_filter(id);
}

void remove_filter(uint32_t id)
{
	context::singleton().remove_filter(id);
}

	} // namespace reporting

// attach to callback in assert.h
assert_action vtracef(const assert_context& ctx, const char* fmt, va_list args)
{
	return reporting::context::singleton().vformatf(ctx, fmt, args);
}

}
