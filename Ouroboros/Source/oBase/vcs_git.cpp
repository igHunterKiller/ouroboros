// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/vcs_git.h>
#include <oString/path.h>

namespace ouro {

vcs_protocol git_t::protocol() const
{
	return vcs_protocol::git;
}

bool git_t::available() const
{
	invoke("git help");
	return !!strstr(stdout_, "usage: git [--version]");
}

bool git_t::is_up_to_date(const char* path, const vcs_revision& at_revision) const
{
	invoke("git -C %s status", path);
	return !!strstr(stdout_, "is up-to-date with");
}

char* git_t::root(char* dst, size_t dst_size, const char* path) const
{
	// git doesn't like filenames for this command, so prune them
	path_t p(path);

	// obviously a filename, so remove it
	if (p.has_extension())
		p.remove_filename();

	// go up the chain in case it's an untracked path or a filename with no extension
	int err = 0;
	while (1)
	{
		err = invoke_nothrow("git -C %s rev-parse --show-toplevel", p.c_str());
		if (!err || !p.has_parent_path())
			break;

		p.remove_filename();
	}

	if (err)
		throw_invoke_err(err);

	// ensure the newline appended in vcs_get_line isn't copied
	char* newline = stdout_.c_str() + strcspn(stdout_, oNEWLINE);
	if (newline)
		*newline = '\0';

	return strlcpy(dst, stdout_, dst_size) < dst_size ? dst : nullptr;
}

vcs_revision git_t::revision(const char* path) const
{
	invoke("git -C %s log -n 1", path);
	const char* rev = strstr(stdout_, "commit");
	oCheck(rev, std::errc::protocol_error, "keyword 'commit' not found");
	rev += 7; // move past 'commit '
	const char* end = rev + strcspn(rev, oNEWLINE);
	return vcs_revision(vcs_protocol::git, rev, end);
}

void git_t::status(const char* path, vcs_enumerate_fn enumerator, void* user, vcs_visit_option option, const vcs_revision& up_to_revision) const
{
	// note: this will respect git's .gitignore - is there an optional way around that?
	// note: haven't thought about few files out-of-date (i.e. force-synced to some prior version)

	if (!enumerator)
		return;

	// status results come back relative to root, so seed with root path
	vcs_file file;
	root(file.path, file.path.capacity(), path);
	file.path.append("/");
	size_t root_len = file.path.length();

	// invoke status to fill stdout_
	{
		const char* uopt = "normal";

		if (option == vcs_visit_option::modified)
			uopt = "no";

		invoke("git -C %s status -z -u%s", path, uopt);
	}

	char* cur = stdout_.c_str();
	while (*cur)
	{
		// parse line: 2 chars for type, a space, then a path to the file relative to the repository root. 
		// The -z nul-terminated-ness comes out through get_line callbacks

		// parse status
		{
			char X = *cur++; // if conflict: left status else status of index
			char Y = *cur++; // if conflict: right status else status of work tree

			// not wholly sure about the rules here, investigate further
			if (X != ' ')
				Y = X;

			vcs_status status = vcs_status::unknown;
			switch (Y)
			{
				case ' ': status = vcs_status::unmodified;  break;
				case 'M': status = vcs_status::modified;	  break;
				case 'A': status = vcs_status::added;			  break;
				case 'D': status = vcs_status::removed;		  break;
				case 'R': status = vcs_status::renamed;		  break;
				case 'C': status = vcs_status::copied;      break;
				case 'U': status = vcs_status::unmerged;    break;
				case '?': status = vcs_status::untracked;   break;
				default: break;
			}

			file.status = status;
		}

		// parse path relative to root
		{
			cur = cur + strspn(cur, oWHITESPACE);
			char* end = cur + strcspn(cur, oNEWLINE);

			// reset to root path
			file.path[root_len] = '\0';

			file.path.append(cur, end);
			cur = end + strspn(end, oNEWLINE);
		}

		// parse revision
		{
			file.revision = vcs_revision(); // todo
		}

		enumerator(file, user);
	}
}

void git_t::add(const char** paths, size_t num_paths) const
{
	for (size_t i = 0; i < num_paths; i++)
		invoke("git add \"%s\"", paths[i]);
}

void git_t::edit(const char** paths, size_t num_paths) const
{
	// noop for git
}

void git_t::remove(const char** paths, size_t num_paths) const
{
	oThrow(std::errc::function_not_supported, "");
}

void git_t::revert(const char** paths, size_t num_paths) const
{
	for (size_t i = 0; i < num_paths; i++)
		invoke("git checkout HEAD %s", paths[i]);
}

void git_t::commit(const char** paths, size_t num_paths) const
{
	oThrow(std::errc::function_not_supported, "");
}

}
