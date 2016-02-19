// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include "scc_svn.h"

#include <oCore/assert.h>
#include <oCore/countof.h>
#include <oCore/stringf.h>
#include <oString/stringize.h>
#include <oString/string_path.h>

namespace ouro {
	namespace detail {

#define SVN_CMDf(_Format, ...) \
	char cmd[512]; \
	if (-1 == snprintf(cmd, _Format, ## __VA_ARGS__)) \
		throw scc_exception(scc_error::command_string_too_long)

#define SVNf(_Format, ...) \
	SVN_CMDf(_Format, ## __VA_ARGS__); \
	xlstring StdOut; \
	spawn(cmd, StdOut)

#define SVN_THROWE(_ExitCode) throw scc_exception(scc_error::scc_exe_error, stringf("svn exited with code %d", _ExitCode))
#define SVN_THROWO(_ExitCode, _StdOut) throw scc_exception(scc_error::scc_exe_error, stringf("svn exited with code %d\n  stdout: %s", _ExitCode, _StdOut.c_str()))
#define SVN_THROWF(_SCCError, _Format, ...) throw scc_exception(scc_error::_SCCError, stringf(_Format, ## __VA_ARGS__ ) )

std::shared_ptr<scc> make_scc_svn(scc_spawn_fn _Spawn, void* _User, unsigned int _TimeoutMS)
{
	return std::make_shared<scc_svn>(_Spawn, _User, _TimeoutMS);
}

static bool svn_is_error(const char* _StdOut)
{
	const char* sErrors[] =
	{
		"not recognized",
		"not a working copy",
		"was not found",
		"Could not display info",
		"has no committed revision",
		"does not exist",
		"Error resolving",
		"E731001",
	};

	for(size_t i = 0; i < countof(sErrors); i++)
		if (strstr(_StdOut, sErrors[i]))
			return true;
	return false;
}

void scc_svn::spawn(const char* _Command, scc_get_line_fn _GetLine, void* _User) const
{
	int ec = Spawn(_Command, _GetLine, _User, SpawnTimeoutMS);
	if (ec)
		SVN_THROWE(ec);
}

void get_line_xl(char* _Line, void* _User)
{
	auto& std_out = *(xlstring*)_User;
	strlcat(std_out, _Line, std_out.capacity());
	strlcat(std_out, "\n",  std_out.capacity());
}

void scc_svn::spawn(const char* _Command, xlstring& _StdOut) const
{
	_StdOut.clear();
	int ec = Spawn(_Command, get_line_xl, &_StdOut, SpawnTimeoutMS);
	if (ec || svn_is_error(_StdOut))
		SVN_THROWO(ec, _StdOut);
}

bool scc_svn::available() const
{
	xlstring StdOut;
	int ec = Spawn("svn", get_line_xl, &StdOut, SpawnTimeoutMS);
	if (ec)
		SVN_THROWE(ec);
	return !!strstr(StdOut, "Type 'svn help'");
}

char* scc_svn::root(const char* _Path, char* _StrDestination, size_t _SizeofStrDestination) const
{
	SVNf("svn info \"%s\"", _Path);

	static const size_t PathKeyLen = 24;
	char* path = strstr(StdOut, "Working Copy Root Path: ");
	if (!path)
		SVN_THROWF(entry_not_found, "%s", "no working copy root path entry found");

	path += PathKeyLen;
	size_t len = strcspn(path, oNEWLINE); // move to end of line

	if (len >= _SizeofStrDestination)
		throw std::invalid_argument("destination buffer to receive scc root is too small");

	memcpy(_StrDestination, path, len);
	_StrDestination[len] = 0;

	if (!clean_path(_StrDestination, _SizeofStrDestination, _StrDestination))
		throw std::invalid_argument("destination buffer to receive scc root is too small");
	return _StrDestination;
}

unsigned int scc_svn::revision(const char* _Path) const
{
	unsigned int r = 0;

	SVNf("svn info \"%s\"", _Path);
	static const size_t RevKeyLen = 10;
	char* rev = strstr(StdOut, "Revision: ");
	if (!rev)
		return 0;

	rev += RevKeyLen;
	size_t len = strcspn(rev, oNEWLINE); // move to end of line
	
	sstring s;
	if (len >= s.capacity())
		return r;

	memcpy(s, rev, len);
	s[len] = 0;

	from_string(&r, s);

	return r;
}

static scc_status::value get_status(char _Status)
{
	switch (_Status)
	{
		case ' ': return scc_status::unchanged;
		case 'A': return scc_status::added;
		case 'C': return scc_status::conflict;
		case 'D': return scc_status::removed;
		case 'I': return scc_status::ignored;
		case 'M': return scc_status::modified;
		case 'R': return scc_status::replaced;
		case 'X': return scc_status::unversioned;
		case '?': return scc_status::unversioned;
		case '!': return scc_status::missing;
		case '~': return scc_status::obstructed;
		default: break;
	}
	return scc_status::unknown;
}

// returns new beginning into the status buffer. The contents of _StatusBuffer
// are altered by this call.
static char* svn_parse_status_line(char* _StatusBuffer, unsigned int _UpToRevision, scc_file& _File)
{
	char* s = _StatusBuffer;

	if (!*s) return nullptr;
	if (!strncmp(s, "Status against", 14)) return nullptr;
	if (strstr(s, "> moved")) // > moved to <some path> skip this for now
	{
		s += strcspn(s, oNEWLINE);
		s += 1 + strspn(s, oWHITESPACE oNEWLINE);
		return *s ? s : nullptr;
	}

	_File.status = get_status(s[0]);
	oASSERT(s[1] == ' ' || (s[0] == 's' && s[1] == 'v' && s[2] == 'n'), "haven't thought about this yet:\n s[1]=%c %s", s[1], _StatusBuffer);
	if (s[8] == '*') _File.status = scc_status::out_of_date;
	s += 9;
	s += strspn(s, oWHITESPACE);
	char* rev = s;
	s += strcspn(s, oWHITESPACE);
	*s++ = 0;
	_File.revision = 0;
	ouro::from_string(&_File.revision, rev);
	if (_UpToRevision != 0 && _File.revision > _UpToRevision) _File.status = scc_status::out_of_date;
	s += strspn(s, oWHITESPACE);
	char* p = s;
	s += strcspn(s, oNEWLINE);
	
	if (*s != '\n' && *s != '\r' && *s != '\0')
		int i = 0;
	
	*s++ = 0;
	clean_path(_File.path, p);
	s += strspn(s, oWHITESPACE oNEWLINE);
	if (!*s) return nullptr;

	return s;
}

struct status_context
{
	xlstring line;
	scc_error::value errc;
	std::string errline;
	scc_file_enumerator_fn file_enumerator;
	void* user;
	unsigned int up_to_revisition;
};

void scc_svn::status(const char* _Path, unsigned int _UpToRevision, scc_visit_option::value _Option, scc_file_enumerator_fn _Enumerator, void* _User) const
{
	char cmd[512];
	
	const char* opt = "";
	switch (_Option)
	{
		case scc_visit_option::visit_all: opt = " -v"; break;
		case scc_visit_option::skip_unversioned: opt = " -v -q"; break;
		case scc_visit_option::skip_unmodified: opt = ""; break;
		case scc_visit_option::modified_only: opt = " -q"; break;
		default: break;
	}

	if (-1 == snprintf(cmd, "svn status -u%s \"%s\"", opt, _Path))
		throw scc_exception(scc_error::command_string_too_long);

	status_context ctx;
	ctx.errc = scc_error::none;
	ctx.file_enumerator = _Enumerator;
	ctx.user = _User;
	ctx.up_to_revisition = _UpToRevision;
	spawn(cmd, [](char* _Line, void* _User)
	{
		auto& ctx = *(status_context*)_User;

		if (ctx.errc == scc_error::none)
		{
			if (svn_is_error(_Line))
			{
				ctx.errc = scc_error::scc_exe_error;
				ctx.errline = _Line;
			}
			else
			{
				scc_file file;
				svn_parse_status_line(_Line, ctx.up_to_revisition, file);
				ctx.file_enumerator(file, ctx.user);
			}
		}
	}, &ctx);

	if (ctx.errc != scc_error::none)
		throw scc_exception(ctx.errc, ctx.errline);
}

static bool svn_from_string(ntp_date* _pDate, const char* _String)
{
	int timezone = 0;
	date d;
	if (7 != sscanf_s(_String, "%d-%d-%d %d:%d:%d %d", &d.year, &d.month, &d.day, &d.hour, &d.minute, &d.second, &timezone))
		return false;
	timezone = (timezone / 100) * 60 * 60;

	*_pDate = date_cast<ntp_date>(d);
	_pDate->hi(_pDate->hi() - timezone);
	return true;
}

static char* svn_to_string(char* _StrDestination, size_t _SizeofStrDestination, const ntp_date& _Date)
{
	return (strftime(_StrDestination, _SizeofStrDestination, sortable_date_format, _Date) < _SizeofStrDestination)
		? _StrDestination : nullptr;
}
template<size_t size> char* svn_to_string(char (&_StrDestination)[size], const ntp_date& _Date) { return svn_to_string(_StrDestination, size, _Date); }
template<size_t capacity> char* svn_to_string(fixed_string<char, capacity>& _StrDestination, const ntp_date& _Date) { return svn_to_string(_StrDestination, _StrDestination.capacity(), _Date); }

scc_revision scc_svn::change(const char* _Path, unsigned int _Revision) const
{
	char cmd[512];
	if (_Revision)
	{
		if (-1 == snprintf(cmd, "svn log -r %d \"%s\"", _Revision, _Path))
			throw scc_exception(scc_error::command_string_too_long);
	}

	else if (-1 == snprintf(cmd, "svn log \"%s\"", _Path))
		throw scc_exception(scc_error::command_string_too_long);

	xlstring StdOut;
	spawn(cmd, StdOut);

	char* rev = strchr(StdOut, 'r');
	if (!rev)
		throw scc_exception(scc_error::entry_not_found);
	rev++;
	char* strWho = strchr(rev, '|');
	*(strWho-1) = 0;
	strWho += 2;
	char* strWhen = strchr(strWho, '|');
	*(strWhen-1) = 0;
	strWhen += 2;
	char* strWhat = strchr(strWhen, '(');
	*(strWhat-1) = 0;
	strWhat += strcspn(strWhat, oNEWLINE);
	strWhat += strspn(strWhat, oNEWLINE oWHITESPACE);
	char* strEnd = strWhat + strcspn(strWhat, oNEWLINE);
	*strEnd = 0;

	scc_revision r;
	ouro::from_string(&r.revision, rev);
	r.who = strWho;
	svn_from_string(&r.when, strWhen);
	r.what = strWhat;

	return r;
}

void scc_svn::sync(const char* _Path, unsigned int _Revision, bool _Force)
{
	sstring StrForce(_Force ? " --force" : "");
	char cmd[512];
	if (_Revision)
	{
		if (-1 == snprintf(cmd, "svn update%s -r %d \"%s\"", StrForce.c_str(), _Revision, _Path))
			throw scc_exception(scc_error::command_string_too_long);
	}

	else if (-1 == snprintf(cmd, "svn update \"%s\"", _Path))
		throw scc_exception(scc_error::command_string_too_long);

	xlstring StdOut;
	spawn(cmd, StdOut);
}

void scc_svn::sync(const char* _Path, const ntp_date& _Date, bool _Force)
{
	const char* StrForce = _Force ? " --force" : "";
	sstring StrDate;
	char cmd[512];
	if (-1 == snprintf(cmd, "svn update%s -r {%d} \"%s\"", StrForce, svn_to_string(StrDate, _Date), _Path))
		throw scc_exception(scc_error::command_string_too_long);
	xlstring StdOut;
	spawn(cmd, StdOut);
}

void scc_svn::add(const char* _Path)
{
	SVNf("svn add \"%s\"", _Path);
}

void scc_svn::remove(const char* _Path, bool _Force)
{
	SVNf("svn delete \"%s\"", _Path);
}

void scc_svn::edit(const char* _Path)
{
	// noop (svn is sandbox, always-writable)
}

void scc_svn::revert(const char* _Path)
{
	SVNf("snv revert \"%s\"", _Path);
}

	} // namespace detail
}
