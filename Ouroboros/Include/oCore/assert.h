// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// support for oASSERT and oTRACE debugging macros. These evaluate to a single
// externally defined ouro::vtracef to be defined by a system implementation.

#pragma once

// _____________________________________________________________________________
// Configuration: Define these macros to control what checks are done.

#ifndef oHAS_oASSERT
	#ifdef oDEBUG
		#define oHAS_oASSERT 1
	#else
		#define oHAS_oASSERT 0
	#endif
#endif

#ifndef oHAS_oASSERTA
	#define oHAS_oASSERTA 0
#endif

#if oHAS_oASSERT == 1 && oHAS_oASSERTA != 1
	#undef oHAS_oASSERTA
	#define oHAS_oASSERTA 1
#endif

#ifndef oHAS_oTRACE
	#define oHAS_oTRACE oHAS_oASSERT
#endif

#ifndef oHAS_oTRACEA
	#define oHAS_oTRACEA oHAS_oASSERTA
#endif

#if oHAS_oTRACE == 1 && oHAS_oTRACEA != 1
	#undef oHAS_oTRACEA
	#define oHAS_oTRACEA 1
#endif

#include <cstdarg>
#include <cstdlib>

// _____________________________________________________________________________
// Compiler-specific way to trigger a breakpoint

#ifdef _MSC_VER
	#define oDEBUGBREAK __debugbreak()
#endif

// _____________________________________________________________________________
// External interface. The macro inserts calls to vtracef even in low-level
// code and it is the client code's responsibility to convey the messasge.

namespace ouro {

	enum class assert_type   : unsigned short { trace, assertion };
	enum class assert_action : unsigned short { abort, debug, ignore, ignore_always };

	struct assert_context
	{
		const char* expression;
		const char* function;
		const char* filename;
		int line;
		assert_type type;
		assert_action default_response;
	};

	// This function is inserted by oAssert macros. It is not implemented: another platform-specific
	// module must implement its functionality.
	extern assert_action vtracef(const assert_context& assertion, const char* format, va_list args);
	inline assert_action  tracef(const assert_context& assertion, const char* format, ...) { va_list args; va_start(args, format); assert_action action = vtracef(assertion, format, args); va_end(args); return action; }
}

// _____________________________________________________________________________
// Main macro wrapper for print callback that ensures a break points to the 
// assertion itself rather than inside some debug function.

#if oHAS_oASSERT == 1 || oHAS_oASSERTA == 1 || oHAS_oTRACE == 1 || oHAS_oTRACEA == 1
	#define oASSERT_TRACE(_type, _default_response, _str_expr, format, ...) do \
	{	static bool oAssert_IgnoreFuture = false; \
		if (!oAssert_IgnoreFuture) \
		{	::ouro::assert_context assertion__; assertion__.expression = _str_expr; assertion__.function = __FUNCTION__; assertion__.filename = __FILE__; assertion__.line = __LINE__; assertion__.type = ::ouro::assert_type::_type; assertion__.default_response = ::ouro::assert_action::_default_response; \
			::ouro::assert_action action__ = ::ouro::tracef(assertion__, format "\n", ## __VA_ARGS__); \
			switch (action__) { case ::ouro::assert_action::abort: abort(); break; case ::ouro::assert_action::debug: oDEBUGBREAK; break; case ::ouro::assert_action::ignore_always: oAssert_IgnoreFuture = true; break; default: break; } \
		} \
	} while(false)
#endif

// _____________________________________________________________________________
// Always-macros (debug or release)

#if oHAS_oASSERTA == 1 || oHAS_oASSERT == 1
	#define oASSERTA(expr, format, ...) do { if (!(expr)) { oASSERT_TRACE(assertion, abort, #expr, format, ## __VA_ARGS__); } } while(false)
#else
	#define oASSERTA(format, ...) do {} while(false)
#endif

#if oHAS_oTRACEA == 1 || oHAS_oTRACE == 1
	#define oTRACEA(format, ...)      oASSERT_TRACE(trace, ignore,        "", format, ## __VA_ARGS__)
	#define oTRACEA_ONCE(format, ...) oASSERT_TRACE(trace, ignore_always, "", format, ## __VA_ARGS__)
#else
	#define oTRACEA(format, ...)      do {} while(false)
	#define oTRACEA_ONCE(format, ...) do {} while(false)
#endif

// _____________________________________________________________________________
// Debug-only macros

#if oHAS_oASSERT == 1
	#define oASSERT(expr, format, ...) do { if (!(expr)) { oASSERT_TRACE(assertion, abort, #expr, format, ## __VA_ARGS__); } } while(false)
#else
	#define oASSERT(format, ...) do {} while(false)
#endif

#if oHAS_oTRACE == 1
	#define oTRACE(format, ...)      oASSERT_TRACE(trace, ignore,        "", format, ## __VA_ARGS__)
	#define oTRACE_ONCE(format, ...) oASSERT_TRACE(trace, ignore_always, "", format, ## __VA_ARGS__)
#else
	#define oTRACE(format, ...)      do {} while(false)
	#define oTRACE_ONCE(format, ...) do {} while(false)
#endif
