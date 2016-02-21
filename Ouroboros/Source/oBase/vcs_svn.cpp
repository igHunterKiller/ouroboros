// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/vcs_svn.h>
#include <oString/string_path.h>

namespace ouro {

vcs_protocol svn_t::protocol() const
{
	return vcs_protocol::svn;
}

bool svn_t::available() const
{
	invoke("svn help");
	return !!strstr(stdout_, "usage: svn <subcommand>");
}

bool svn_t::is_up_to_date(const char* path, const vcs_revision& at_revision) const
{
	return false;
}

char* svn_t::root(char* dst, size_t dst_size, const char* path) const
{
	invoke("svn info \"%s\"", path);

	char* cur = strstr(stdout_, "Working Copy Root Path: ");
	if (!cur)
		throw vcs_exception(vcs_error::entry_not_found, stringf("%s", "no working copy root path entry found"));

	size_t len = strcspn(cur + 24, oNEWLINE);
	oCheck(len < dst_size, std::errc::no_buffer_space, "destination buffer to receive vcs root is too small");

	memcpy(dst, path, len);
	dst[len] = 0;

	oCheck(clean_path(dst, dst_size, dst), std::errc::invalid_argument, "clean_path failed");
	return dst;
}

vcs_revision svn_t::revision(const char* path) const
{
	invoke("svn info \"%s\"", path);
	char* rev = strstr(stdout_, "Revision: ");
	oCheck(rev, std::errc::protocol_error, "keyword 'Revision' not found");
	rev += 10; // 'Revision: '
	char* end = rev + strcspn(rev, oNEWLINE);
	return vcs_revision(vcs_protocol::svn, rev, end);
}

void svn_t::status(const char* path, vcs_enumerate_fn enumerator, void* user, vcs_visit_option option, const vcs_revision& up_to_revision) const
{
	if (!enumerator)
		return;

	const char* opt = "";
	switch (option)
	{
		// -v does 'all'
		case vcs_visit_option::modified:               opt = " -q"; break;
		case vcs_visit_option::modified_and_untracked: opt = "";    break;
		default: break;
	}

	invoke("svn status -u%s \"%s\"", opt, path);

	vcs_file file;

	char* cur = stdout_.c_str();

	while (*cur)
	{
		if (!strncmp(cur, "Status against", 14)) // last line, so consider this an exit flag
			break;

		if (strstr(cur, "> moved")) // > moved to <some path> skip this for now
		{
			cur += strcspn(cur, oNEWLINE);
			cur += 1 + strspn(cur, oWHITESPACE oNEWLINE);
			continue;
		}

		// process status
		{
			char status_code = cur[0];

			vcs_status status = vcs_status::unknown;
			switch (status_code)
			{
				case ' ': status = vcs_status::unmodified; break;
				case 'A': status = vcs_status::added;			 break;
				case 'C': status = vcs_status::conflict;	 break;
				case 'D': status = vcs_status::removed;		 break;
				case 'I': status = vcs_status::ignored;		 break;
				case 'M': status = vcs_status::modified;	 break;
				case 'R': status = vcs_status::replaced;	 break;
				case 'X': status = vcs_status::untracked;	 break;
				case '?': status = vcs_status::untracked;	 break;
				case '!': status = vcs_status::missing;		 break;
				case '~': status = vcs_status::obstructed; break;
				default: break;
			}

			oAssert(cur[1] == ' ' || (cur[0] == 'cur' && cur[1] == 'v' && cur[2] == 'n'), "haven't thought about this yet:\n cur[1]=%c %s", cur[1], stdout_.c_str());
			if (cur[8] == '*') status = vcs_status::out_of_date;

			file.status = status;
		}

		// process revision
		{
			cur += strspn(cur + 9, oWHITESPACE);
			char* rev = cur;
			cur += strcspn(cur, oWHITESPACE);
			*cur++ = 0;
			file.revision.from_string(rev);

			if (!!up_to_revision && file.revision > up_to_revision)
				file.status = vcs_status::out_of_date;
		}

		// process filename
		{
			cur += strspn(cur, oWHITESPACE);
			char* p = cur;
			cur += strcspn(cur, oNEWLINE);
			*cur++ = 0;
			clean_path(file.path, p);
			cur += strspn(cur, oWHITESPACE oNEWLINE);
		}

		enumerator(file, user);
	}
}

void svn_t::add(const char** paths, size_t num_paths) const
{
	for (size_t i = 0; i < num_paths; i++)
		invoke("svn add \"%s\"", paths[i]);
}

void svn_t::edit(const char** paths, size_t num_paths) const
{
	// noop for svn
}

void svn_t::remove(const char** paths, size_t num_paths) const
{
	for (size_t i = 0; i < num_paths; i++)
		invoke("svn delete \"%s\"", paths[i]);
}

void svn_t::revert(const char** paths, size_t num_paths) const
{
	for (size_t i = 0; i < num_paths; i++)
		invoke("svn revert \"%s\"", paths[i]);
}

void svn_t::commit(const char** paths, size_t num_paths) const
{
	oThrow(std::errc::function_not_supported, "");
}

}
