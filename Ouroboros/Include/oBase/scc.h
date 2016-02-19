// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Abstraction for source code control.

#pragma once
#include <functional>
#include <oBase/date.h>
#include <oString/fixed_string.h>
#include <memory>

namespace ouro {

namespace scc_error
{	enum value {

	none,
	command_string_too_long,
	scc_exe_not_available,
	scc_exe_error,
	scc_server_not_available,
	file_not_found,
	entry_not_found, // returned when parsing output fails

};}

const std::error_category& scc_category();

/*constexpr*/ inline std::error_code make_error_code(scc_error::value err) { return std::error_code(static_cast<int>(err), scc_category()); }
/*constexpr*/ inline std::error_condition make_error_condition(scc_error::value err) { return std::error_condition(static_cast<int>(err), scc_category()); }

class scc_exception : public std::system_error
{
public:
	scc_exception(const scc_error::value& err) : std::system_error(make_error_code(err)) {}
	scc_exception(const scc_error::value& err, const std::string& what) : std::system_error(make_error_code(err), what) {}
};

namespace scc_protocol
{	enum value {

	p4,
	svn,
	git,

};}

namespace scc_status
{	enum value {

	unknown,
	unchanged,
	unversioned,
	ignored,
	modified,
	missing,
	added,
	removed,
	replaced,
	copied,
	conflict,
	merged,
	obstructed,
	out_of_date,

};}

namespace scc_visit_option
{	enum value {

	visit_all,
	skip_unversioned,
	skip_unmodified,
	modified_only,

};}

struct scc_file
{
	scc_file()
		: status(scc_status::unknown)
		, revision(~0u)
	{}

	path_string path;
	scc_status::value status;
	unsigned int revision;
};

struct scc_revision
{
	scc_revision()
		: revision(~0u)
	{}

	unsigned int revision;
	mstring who;
	ntp_date when;
	xlstring what;
};

typedef void (*scc_get_line_fn)(char* line, void* user);
typedef int (*scc_spawn_fn)(const char* _Commandline, scc_get_line_fn get_line, void* _User, unsigned int _TimeoutMS);
typedef void (*scc_file_enumerator_fn)(const scc_file& file, void* user);

class scc
{
public:
	// Returns the current protocol in use by this instances
	virtual scc_protocol::value protocol() const = 0;

	// Returns true if the client executable for the specified protocol can be 
	// executed by the scc_spawn function.
	virtual bool available() const = 0;

	// Returns the root of the current tree
	virtual char* root(const char* _Path, char* _StrDestination, size_t _SizeofStrDestination) const = 0;
	template<size_t size> char* root(const char* _Path, char (&_StrDestination)[size]) const { return root(_Path, _StrDestination, size); }
	template<size_t capacity> char* root(const char* _Path, ouro::fixed_string<char, capacity>& _StrDestination) const { return root(_Path, _StrDestination, _StrDestination.capacity()); }

	// Returns the basic version that the path would be at if there were no 
	// modified files and all were up-to-date. If there is a failure this returns 
	// 0 (zero).
	virtual unsigned int revision(const char* _Path) const = 0;

	// Visits each file that matches the search under the specified path and given 
	// the specified option.
	virtual void status(const char* _Path, unsigned int _UpToRevision, scc_visit_option::value _Option, scc_file_enumerator_fn _Visitor, void* _User) const = 0;

	// Returns true if all files under the specified path are unmodified and at 
	// their latest revisions up to the specified revision so this can be used 
	// on historical changelists for builds after other commits are done. Passing
	// zero implies the head revision.
	inline bool is_up_to_date(const char* _Path, unsigned int _AtRevision = 0) const
	{
		bool UpToDate = true;
		status(_Path, _AtRevision, scc_visit_option::modified_only, [](const scc_file& _File, void* _User)
		{
			bool& UpToDate = *(bool*)_User;

			if (_File.status == scc_status::out_of_date || _File.status == scc_status::modified)
				UpToDate = false;
		}, &UpToDate);

		return UpToDate;
	}

	// Returns information about the specified change. If 0, this will return the
	// latest/head revision.
	virtual scc_revision change(const char* _Path, unsigned int _Revision) const = 0;

	// Sync the specified path to the specified revision or date. Revision 0 will 
	// sync to the head revision.
	virtual void sync(const char* _Path, unsigned int _Revision, bool _Force = false) = 0;
	virtual void sync(const char* _Path, const ntp_date& _Date, bool _Force = false) = 0;

	virtual void add(const char* _Path) = 0;

	virtual void remove(const char* _Path, bool _Force = false) = 0;
	
	// for checkout-style scc like perforce. This noops on sandbox-style scc.
	virtual void edit(const char* _Path) = 0;

	virtual void revert(const char* _Path) = 0;
};

std::shared_ptr<scc> make_scc(scc_protocol::value _Protocol, scc_spawn_fn _Spawn, void* _User = nullptr, unsigned int _TimeoutMS = 5000);

}
