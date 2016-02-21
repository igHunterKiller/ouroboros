// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// version control system abstraction

#pragma once
#include <oString/fixed_string.h>
#include <system_error>

namespace ouro {

// _____________________________________________________________________________
// std version control system error exception

enum class vcs_error
{
	none,
	command_string_too_long,
	exe_not_available,
	exe_error,
	server_not_available,
	file_not_found,
	entry_not_found, // returned when parsing output fails

	count,
};

const std::error_category& vcs_category();

/*constexpr*/ inline std::error_code make_error_code(vcs_error err) { return std::error_code(static_cast<int>(err), vcs_category()); }
/*constexpr*/ inline std::error_condition make_error_condition(vcs_error err) { return std::error_condition(static_cast<int>(err), vcs_category()); }

class vcs_exception : public std::system_error
{
public:
	vcs_exception(const vcs_error& err) : std::system_error(make_error_code(err)) {}
	vcs_exception(const vcs_error& err, const std::string& what) : std::system_error(make_error_code(err), what) {}
};


// _____________________________________________________________________________
// vcs enumerations and structs

enum class vcs_protocol : uint8_t
{
	invalid,
	git,
	p4,
	svn,

	count,
};

enum class vcs_status : uint8_t
{
	unknown,
	unmodified,
	untracked,
	ignored,
	modified,
	missing,
	added,
	removed,
	renamed,
	replaced,
	copied,
	conflict,
	unmerged,
	merged,
	obstructed,
	out_of_date,

	count,
};

enum class vcs_visit_option : uint8_t
{
	modified,
	modified_and_untracked,

	count,
};

class vcs_revision
{
	// large to support git's SHA1 20-byte revision hash

public:
  vcs_revision()												                                                           : protocol_(vcs_protocol::count), pada_(0), padb_(0), upper4_(0),      mid8_(0),    lower8_(0)        {}
  vcs_revision(const vcs_protocol& protocol)												                               : protocol_(protocol),            pada_(0), padb_(0), upper4_(0),      mid8_(0),    lower8_(0)        {}
  vcs_revision(const vcs_protocol& protocol, uint16_t upper4, uint64_t mid8, uint64_t lower8)      : protocol_(protocol),            pada_(0), padb_(0), upper4_(upper4), mid8_(mid8), lower8_(lower8)   {}
  vcs_revision(const vcs_protocol& protocol, uint32_t revision)                                    : protocol_(protocol),            pada_(0), padb_(0), upper4_(0),      mid8_(0),    lower8_(revision) {}
  vcs_revision(const vcs_protocol& protocol, const char* revision_string)                          : protocol_(protocol),            pada_(0), padb_(0), upper4_(0),      mid8_(0),    lower8_(0)        { from_string(revision_string); }
  vcs_revision(const vcs_protocol& protocol, const char* revision_start, const char* revision_end) : protocol_(protocol),            pada_(0), padb_(0), upper4_(0),      mid8_(0),    lower8_(0)        { from_string(revision_start, revision_end); }
  const vcs_revision& operator=(const vcs_revision& that)                                                                                                                                                { upper4_ = that.upper4_; mid8_ = that.mid8_; lower8_ = that.lower8_; return *this; }
  const vcs_revision& operator=(const char* revision_string)                                                                                                                                             { from_string(revision_string); return *this; }

	vcs_protocol protocol() const { return protocol_;  }
	operator bool()         const { return upper4_ != 0 || mid8_ != 0 || lower8_ != 0; }
	operator uint32_t()     const { return (uint32_t)lower8_; }

	bool operator< (const vcs_revision& that) const { return (upper4_ < that.upper4_) || (upper4_ == that.upper4_ && mid8_ < that.mid8_) || (upper4_ == that.upper4_ && mid8_ == that.mid8_ && lower8_ < that.lower8_); }
	bool operator> (const vcs_revision& that) const { return that < *this; }
	bool operator>=(const vcs_revision& that) const { return !(*this < that); }
	bool operator<=(const vcs_revision& that) const { return !(*this > that); }
	bool operator==(const vcs_revision& that) const { return upper4_ == that.upper4_ && mid8_ == that.mid8_ && lower8_ == that.lower8_; }
	bool operator!=(const vcs_revision& that) const { return !(*this == that); }

	char* to_string(char* dst, size_t dst_size) const;
	void  from_string(const char* revision_string);
	void  from_string(const char* revision_start, const char* revision_end);

	template<size_t size> char* to_string(char (&dst)[size]) const { return to_string(dst, size); }

private:
	vcs_protocol protocol_;
	uint8_t      pada_;
	uint16_t     padb_;
	uint32_t     upper4_;
	uint64_t     mid8_;
	uint64_t     lower8_;

	void internal_from_string(char revision_string[64], int len);
};

struct vcs_file
{
	vcs_file() : status(vcs_status::unknown) {}

	path_string  path;
	vcs_revision revision;
	vcs_status   status;
};

// points to a line of stdout emitted from the vcs's executable
typedef void (*vcs_get_line_fn)(char* line, void* user);

// function to call to invoke the vcs executable. returns exit code of cmdline
typedef int (*vcs_spawn_fn)(const char* cmdline, vcs_get_line_fn get_line, void* user, uint32_t timeout_ms);

typedef void (*vcs_enumerate_fn)(const vcs_file& file, void* user);

struct vcs_init_t
{
	vcs_init_t() : spawn(nullptr), timeout_ms(5000) {}

	vcs_spawn_fn    spawn;
	uint32_t        timeout_ms;
};


// _____________________________________________________________________________
// primary interface see other vcs headers for the implementations of this api

class vcs_t
{
public:
	// === initialization === 

	void initialize(const vcs_init_t& init);
	void deinitialize();


	// === inspection/accessor api === 

	// returns how long before a invokation of the underlying vcs execuable gives up
	inline uint32_t timeout() const { return init_.timeout_ms; }

	// returns the protocol being used
	virtual vcs_protocol protocol() const = 0;

	// returns true if the protocol executable can be run
	virtual bool available() const = 0;

	// returns true if the specified path has no modifications and all files are at the specified revision (forwards or backwards from there counts as out-of-date)
	virtual bool is_up_to_date(const char* path, const vcs_revision& at_revision = vcs_revision()) const = 0;

	// fills and returns the root of the revision-controlled tree that tracks the specified path, nullptr on failure
	virtual char* root(char* dst, size_t dst_size, const char* path) const = 0;
	template<size_t size> char* root(char (&dst)[size], const char* path) const { return root(dst, size, path); }

	// returns the basic revision the path would be at if all files are unmodified and up-to-date
	virtual vcs_revision revision(const char* path) const = 0;

	// visits each file under the specified path that matches the parameters
	virtual void status(const char* path, vcs_enumerate_fn enumerator, void* user, vcs_visit_option option = vcs_visit_option::modified_and_untracked, const vcs_revision& up_to_revision = vcs_revision()) const = 0;

	virtual void add   (const char** paths, size_t num_paths) const = 0;
	virtual void edit  (const char** paths, size_t num_paths) const = 0;
	virtual void remove(const char** paths, size_t num_paths) const = 0;
	virtual void revert(const char** paths, size_t num_paths) const = 0;
	virtual void commit(const char** paths, size_t num_paths) const = 0;

	// todo: sync to [latest|specific rev|specific|date]
	// todo: log()

protected:
	vcs_init_t init_;

	// temp buffers to prevent large stack usage or dynamic allocs, 
	// so const methods still spiritually applies
	mutable fixed_string<char,  1*1024> cmdline_;
	mutable fixed_string<char, 16*1024> stdout_;

	vcs_t() {}
	virtual ~vcs_t() { deinitialize(); }

	int invoke_nothrow(const char* fmt, ...) const;
	void throw_invoke_err(int errc) const;
	void invoke(const char* fmt, ...) const;
};

}
