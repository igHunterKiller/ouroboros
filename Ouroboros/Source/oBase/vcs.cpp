// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/stringize.h>
#include <oBase/vcs.h>
#include <oString/path.h>
#include <oString/string.h>

namespace ouro {

template<> const char* as_string(const vcs_error& e)
{
	static const char* s_names[] =
	{
		"none",
		"command_string_too_long",
		"exe_not_available",
		"exe_error",
		"server_not_available",
		"file_not_found",
		"entry_not_found",
	};
	return as_string(e, s_names);
}

template<> const char* as_string(const vcs_protocol& p)
{
	static const char* s_names[] =
	{
		"unknown",
		"git",
		"perforce",
		"svn",
	};
	return as_string(p, s_names);
}

template<> const char* as_string(const vcs_status& s)
{
	static const char* s_names[] =
	{
		"unknown",
		"unmodified",
		"untracked",
		"ignored",
		"modified",
		"missing",
		"added",
		"removed",
		"renamed",
		"replaced",
		"copied",
		"conflict",
		"unmerged",
		"merged",
		"obstructed",
		"out_of_date",
	};
	return as_string(s, s_names);
}

template<> const char* as_string(const vcs_visit_option& o)
{
	static const char* s_names[] =
	{
		"modified",
		"modified_and_untracked",
	};
	return as_string(o, s_names);
}

class vcs_category_impl : public std::error_category
{
public:
	const char* name() const noexcept override { return "version control system"; }
	std::string message(int errc) const override
	{
		static const char* s_msgs[] =
		{
			"no error",
			"the command string is too long for internal buffers",
			"vcs executable not available",
			"vcs exe reported an error",
			"vcs server not available",
			"file not found",
			"entry not found",
		};
		return as_string((vcs_error)errc, s_msgs);
	}
};

const std::error_category& vcs_category()
{
	static vcs_category_impl s_singleton;
	return s_singleton;
}

char* vcs_revision::to_string(char* dst, size_t dst_size) const
{
	if (!*this)
	{
		strlcpy(dst, "???", dst_size);
		return dst;
	}

	if (protocol_ == vcs_protocol::git) // hex
	{
		int offset = 0;

		if (upper4_)
			offset = snprintf(dst + offset, dst_size - offset, "%llx", upper4_);

		if (mid8_)
			offset += snprintf(dst + offset, dst_size - offset, upper4_ ? "%016llx" : "%llx", mid8_);

		if (lower8_)
			offset += snprintf(dst + offset, dst_size - offset, (upper4_ || mid8_) ? "%016llx" : "%llx", lower8_);

		if (!offset)
			strlcpy(dst, "0", 1);
	}

	else // decimal
	{
		if (!ouro::to_string(dst, dst_size, lower8_))
			return nullptr;
	}

	return dst;
}

static bool from_string_hex(uint64_t* out_value, const char* src) { return 1 == sscanf_s(src, "%llx", out_value); }

void vcs_revision::internal_from_string(char revision_string[64], int len)
{
	if (!*revision_string)
	{
		*this = vcs_revision(protocol_);
		return;
	}

	if (protocol_ == vcs_protocol::git) // hex revision
	{
		char* lower = revision_string + __max(0, len - 16);
		if (from_string_hex(&lower8_, lower))
		{
			*lower = '\0';
			char* mid = __max(revision_string, lower - 16);
			if (mid != lower)
			{
				if (!from_string_hex(&mid8_, mid))
					goto error;

				*mid = '\0';
				char* upper = __max(revision_string, mid - 16);
				if (upper != mid)
				{
					uint64_t tmp = 0;
					if (!from_string_hex(&tmp, upper))
						goto error;

					upper4_ = (uint16_t)tmp;
				}
			}

			return;
		}
	}

	else // decimal revision (also 32-bit)
	{
		upper4_ = 0;
		mid8_ = 0;
		if (ouro::from_string(&lower8_, revision_string))
			return;
	}

error:
	oThrow(std::errc::invalid_argument, "invalid revision_string \"%s\"", revision_string ? revision_string : "(null)");
}

void vcs_revision::from_string(const char* revision_string)
{
	if (revision_string)
	{
		int len = (int)strlen(revision_string);
		char tmp[64];
		if (len < countof(tmp))
		{
			strlcpy(tmp, revision_string);
			internal_from_string(tmp, len);
			return;
		}
	}

	oThrow(std::errc::invalid_argument, "invalid revision_string \"%s\"", revision_string ? revision_string : "(null)");
}

void vcs_revision::from_string(const char* revision_start, const char* revision_end)
{
	if (revision_start && revision_end)
	{
		int len = (int)(revision_end - revision_start);
		char tmp[64];
		if (len < countof(tmp))
		{
			memcpy(tmp, revision_start, len);
			tmp[len] = '\0';
			internal_from_string(tmp, len);
			return;
		}
	}

	oThrow(std::errc::invalid_argument, "invalid revision string piece starting at \"%s\"", revision_start ? revision_start : "(null)");
}

void vcs_t::initialize(const vcs_init_t& init)
{
	oCheck(init.spawn, std::errc::invalid_argument, "a valid spawn function must be specified");
	init_ = init;
	stdout_.clear();
}

void vcs_t::deinitialize()
{
	init_ = vcs_init_t();
	stdout_.clear();
}

static void vcs_get_line(char* line, void* user)
{
	auto& buffer = *(xlstring*)user;
	buffer.append(line);
	buffer.append("\n");
};

int vcs_t::invoke_nothrow(const char* fmt, ...) const
{
	stdout_.clear();
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(cmdline_, cmdline_.capacity(), fmt, args);
	va_end(args);

	if (n < 0 || n >= (int)cmdline_.capacity())
		return (int)std::errc::no_buffer_space;

	return init_.spawn(cmdline_, vcs_get_line, &stdout_, init_.timeout_ms);
}

void vcs_t::throw_invoke_err(int errc) const
{
	throw vcs_exception(vcs_error::exe_error, stringf("%s return exit code %d (0x%08x) while executing \"%s\" stdout:\n%s", as_string(protocol()), errc, errc, cmdline_.c_str(), stdout_.c_str()));
}

void vcs_t::invoke(const char* fmt, ...) const
{
	stdout_.clear();
	va_list args;
	va_start(args, fmt);
	int n = vsnprintf(cmdline_, cmdline_.capacity(), fmt, args);
	va_end(args);
	if (n < 0 || n >= (int)cmdline_.capacity())
		throw vcs_exception(vcs_error::command_string_too_long);
	int err = init_.spawn(cmdline_, vcs_get_line, &stdout_, init_.timeout_ms);
	if (err)
		throw_invoke_err(err);
}

}