// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oGUI/msgbox.h>

namespace ouro {

static assert_action to_action(msg_result _Result)
{
	switch (_Result)
	{
		case msg_result::abort: return assert_action::abort;
		case msg_result::debug: return assert_action::debug;
		case msg_result::ignore: return assert_action::ignore_always;
		default: break;
	}

	return assert_action::ignore;
}

static assert_action show_msgbox(const assert_context& assertion, msg_type type, const char* string)
{
	#ifdef _DEBUG
		#define MSGBOX_BUILD_TYPE "Debug"
	#else
		#define MSGBOX_BUILD_TYPE "Release"
	#endif
	static const char* DIALOG_BOX_TITLE = "Ouroboros " MSGBOX_BUILD_TYPE " Library";

	char format[16 * 1024];
	char cmdline[2048];
	*format = 0;
	char* end = format + sizeof(format);
	char* cur = format;
	cur += snprintf(format, MSGBOX_BUILD_TYPE " %s!\nFile: %s(%d)\nCommand Line: %s\n"
		, type == msg_type::warn ? "Warning" : "Error"
		, assertion.filename
		, assertion.line
		, this_process::command_line(cmdline)
		, assertion.expression);

	if (oSTRVALID(assertion.expression))
		cur += snprintf(cur, std::distance(cur, end), "Expression: %s\n", assertion.expression);

	*cur++ = '\n';

	strlcpy(cur, string, std::distance(cur, end));

	path_t AppPath = filesystem::app_path(true);
	char title[64];
	snprintf(title, "%s (%s)", DIALOG_BOX_TITLE, AppPath.c_str());
	return to_action(msgbox(type, nullptr, title, "%s", format));
}

assert_action prompt_msgbox(const assert_context& assertion, const char* message)
{
	// Output message
	assert_action action = assertion.default_response;
	switch (assertion.type)
	{
		default:
		case assert_type::trace:
			break;

		case assert_type::assertion:
			action = show_msgbox(assertion, msg_type::debug, message);
			break;
	}

	return action;
}

}
