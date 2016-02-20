// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// file system abstraction

#pragma once
#include <oMemory/allocate.h>
#include <oString/path.h>
#include <cstdio>
#include <memory>
#include <system_error>
#include <cstdint>

namespace ouro { namespace filesystem {

// _____________________________________________________________________________
// std filesystem error exception

template<typename charT, typename TraitsT = default_posix_path_traits<charT>>
class basic_filesystem_error : public std::system_error
{
public:
	typedef basic_path_t<charT, TraitsT> path_type;

	basic_filesystem_error()                                                                                                                                                                              {}
	~basic_filesystem_error()                                                                                                                                                                             {}
	basic_filesystem_error(const basic_filesystem_error<charT, TraitsT>& that)                                       : std::system_error(that),                  path1_(that.path1_), path2_(that.path2_) {}
	basic_filesystem_error(std::error_code errcode)                                                                  : system_error(errcode, errcode.message())                                           {}
	basic_filesystem_error(const path_type& path1, std::error_code errcode)                                          : system_error(errcode, errcode.message()), path1_(path1)                            {}
	basic_filesystem_error(const path_type& path1, const path_type& path2, std::error_code errcode)                  : system_error(errcode, errcode.message()), path1_(path1),       path2_(path2)       {}
	basic_filesystem_error(const char* msg, std::error_code errcode)                                                 : system_error(errcode, msg)                                                         {}
	basic_filesystem_error(const char* msg, const path_type& path1, std::error_code errcode)                         : system_error(errcode, msg),               path1_(path1)                            {}
	basic_filesystem_error(const char* msg, const path_type& path1, const path_type& path2, std::error_code errcode) : system_error(errcode, msg),               path1_(path1),       path2_(path2)       {}
	
	inline const path_type& path1() const { return path1_; }
	inline const path_type& path2() const { return path2_; }

private:
	path_type path1_;
	path_type path2_;
};

typedef basic_filesystem_error<char>                                          filesystem_error;
typedef basic_filesystem_error<wchar_t>                                       wfilesystem_error;
typedef basic_filesystem_error<char, default_windows_path_traits<char>>       windows_filesystem_error;
typedef basic_filesystem_error<wchar_t, default_windows_path_traits<wchar_t>> windows_wfilesystem_error;

const std::error_category& filesystem_category();

/*constexpr*/ inline std::error_code make_error_code(std::errc errc)           { return std::error_code(static_cast<int>(errc), filesystem_category()); }
/*constexpr*/ inline std::error_condition make_error_condition(std::errc errc) { return std::error_condition(static_cast<int>(errc), filesystem_category()); }


// _____________________________________________________________________________
// filesystem enums and structs

enum class open_option    { binary_read, binary_write, binary_append, text_read, text_write, text_append };
enum class load_option    { text_read, binary_read };
enum class save_option    { text_write, text_append, binary_write, binary_append };
enum class copy_option    { fail_if_exists, overwrite_if_exists };
enum class symlink_option { none, no_recurse = none, recurse };
enum class map_option     { binary_read, binary_write };
enum class seek_origin    { set, cur, end };

enum class file_type
{
	block_file,
	character_file,
	directory_file,
	fifo_file,
	file_not_found,
	regular_file,
	socket_file,
	status_unknown,
	symlink_file,
	type_unknown,
	read_only_directory_file,
	read_only_file,

	count,
};

struct space_info
{
	uint64_t available;
	uint64_t capacity;
	uint64_t free;
};

class file_status
{
public:
	explicit file_status(file_type type = file_type::status_unknown) : file_type_(type) {}
	inline file_type type() const { return file_type_; }
	inline void type(const file_type& type) { file_type_ = type; }
private:
	file_type file_type_;
};

// This is retained by async file i/o and called once the i/o is complete. The buffer
// can either be moved and retained or left alone where the system will auto-free it
// when the completion is finished. In success cases syserr will be nullptr but if it 
// is valid then buffer is invalid. Data is a user-specified pointer.
typedef void (*completion_fn)(const path_t& path, blob& buffer, const std::system_error* syserr, void* user);

// Called on each file during enumeration to indicate relevant information including the full
// path, its status and size. Data is a user-specified pointer. This should return true to 
// continue enummeration or false to exit early.
typedef bool (*enumerate_fn)(const path_t& path, const file_status& status, uint64_t size, void* user);


// _____________________________________________________________________________
// system path accessors

// All paths to folders end with a separator
path_t app_path(bool include_filename = false); // includes executable name
path_t temp_path(bool include_filename = false); // includes a uniquely named file
path_t log_path(bool include_filename = false, const char* exe_suffix = nullptr); // includes a name based on the app name, date and process id
path_t desktop_path();
path_t system_path();
path_t os_path();
path_t dev_path(); // development root path
path_t data_path();
path_t current_path();
void   current_path(const path_t& path);
path_t root_path(const char* root_name); // pass a string to call one of the above path accessors: [app temp log desktop system os dev data current]
path_t resolve(const path_t& relative_path); // resolve against system paths in this order: [app current system os desktop data temp] and if no match, against envvar PATH contents


// _____________________________________________________________________________
// file status/metadata inspection and wrappers

uint64_t file_size(const path_t& path);
time_t   last_write_time(const path_t& path);
void     last_write_time(const path_t& path, time_t time); // aka touch
void     read_only(const path_t& path, bool read_only = true); // marks file RO or RW

file_status status(const path_t& path);
inline bool is_directory(const file_status& status)     { return status.type() == file_type::directory_file || status.type() == file_type::read_only_directory_file; }
inline bool is_directory(const path_t& path)            { return is_directory(status(path)); }
inline bool is_read_only_directory(file_status status)  { return status.type() == file_type::read_only_directory_file; }
inline bool is_read_only_directory(const path_t& path)  { return is_read_only_directory(status(path)); }
inline bool is_regular(file_status status)              { return status.type() == file_type::regular_file || status.type() == file_type::read_only_file; }
inline bool is_regular(const path_t& path)              { return is_regular(status(path)); }
inline bool is_read_only_regular(file_status status)    { return status.type() == file_type::read_only_file; }
inline bool is_read_only_regular(const path_t& path)    { return is_read_only_regular(status(path)); }
inline bool is_read_only(file_status status)            { return is_read_only_directory(status) || is_read_only_regular(status); }
inline bool is_read_only(const path_t& path)            { return is_read_only(status(path)); }
inline bool is_symlink(file_status status)              { return status.type() == file_type::symlink_file; }
inline bool is_symlink(const path_t& path)              { return is_symlink(status(path)); }
inline bool exists(file_status status)                  { return status.type() != file_type::status_unknown && status.type() != file_type::file_not_found; }
       bool exists(const path_t& path);
inline bool is_other(file_status status)                { return exists(status) && !is_regular(status) && is_read_only_regular(status) && !is_directory(status) && !is_symlink(status); }
inline bool is_other(const path_t& path)                { return is_other(status(path)); }


// _____________________________________________________________________________
// directory operations (fails for files)

bool remove_directory(const path_t& path);
bool create_directory(const path_t& path); // returns false if dir already exists, throws on failure such as missing parent dirs
bool create_directories(const path_t& path); // create all dirs along the path


// _____________________________________________________________________________
// file or directory operations

       bool     remove_filename(const path_t& path);
inline bool     remove         (const path_t& path) { return remove_filename(path); }
       uint32_t remove_all     (const path_t& path); // supports wildcards, returns number of items removed
       void     rename         (const path_t& from, const path_t& to, copy_option opt = copy_option::fail_if_exists);
       void     copy_file      (const path_t& from, const path_t& to, copy_option opt = copy_option::fail_if_exists);
       uint32_t copy_all       (const path_t& from, const path_t& to, copy_option opt = copy_option::fail_if_exists); // returns number of items copied


// _____________________________________________________________________________
// volume operations

space_info space                (const path_t& path); // returns information for the volume contained in path
void       enumerate            (const path_t& wildcard_path, enumerate_fn enumerator, void* user); // enumerates matching wildcard_path, no recursion
void       enumerate_recursively(const path_t& wildcard_path, enumerate_fn enumerator, void* user); // enumerates matching wildcard_path and recurses into sub-directories


// _____________________________________________________________________________
// memory-mapped files

// Memory-maps the specified file, similar to mmap or MapViewOfFile but with a 
// set of policy constraints to keep its usage simple. The specified path is 
// opened and exposed solely as the mapped pointer and the file is closed when 
// unmapped.
void* map(const path_t& path, map_option opt, uint64_t offset, uint64_t size);
void unmap(void* mapped_ptr);


// _____________________________________________________________________________
// fopen style api

typedef FILE* file_handle;
typedef void* native_file_handle;	

// Converts a standard C file handle from fopen to an OS-native handle (HANDLE on Windows)
native_file_handle         get_native(file_handle hfile);
file_handle open           (const path_t& path, open_option opt);
void        close          (file_handle hfile);
bool        at_end         (file_handle hfile);
path_t      get_path       (file_handle hfile);
void        last_write_time(file_handle hfile, time_t _time);
void        seek           (file_handle hfile, int64_t offset, seek_origin origin);
uint64_t    tell           (file_handle hfile);
uint64_t    file_size      (file_handle hfile);
uint64_t    read           (file_handle hfile, void* dst, uint64_t dst_size, uint64_t read_size);
uint64_t    write          (file_handle hfile, const void* src, uint64_t src_size, bool flush = false);

class scoped_file
{
public:
	scoped_file()                                    : h(nullptr) {}
	scoped_file(const path_t& path, open_option opt) : h(nullptr) { if (!path.empty()) h = open(path, opt); }
	scoped_file(file_handle hfile)                   : h(hfile)   {}
	scoped_file(scoped_file&& that)                               { operator=(std::move(that)); }
	scoped_file& operator=(scoped_file&& that)                    { if (this != &that) { if (h) close(h); h = that.h; that.h = nullptr; } return *this; }
	~scoped_file()                                                { if (h) close(h); }
	operator bool() const                                         { return !!h; }
	operator file_handle() const                                  { return h; }

private:
	file_handle h;
	scoped_file(const scoped_file&);
	const scoped_file& operator=(const scoped_file&);
};


// _____________________________________________________________________________
// utility functions wrapping fopen apis

// Writes the entire buffer to the specified file. Open is specified to allow 
// text and write v. append specification.
       void save(const path_t& path, const void* src, size_t src_size, save_option opt = save_option::binary_write);
inline void save(const path_t& path, const blob& buffer, save_option opt = save_option::binary_write) { save(path, buffer, buffer.size(), opt); }

// Allocates the size of the file reads it into memory and returns that buffer.
// If loaded as text, the allocation will be padded and a nul terminator will be
// added so the buffer can immediately be used as a string.
       blob load(const path_t& path, load_option opt = load_option::binary_read, const allocator& alloc = default_allocator);
inline blob load(const path_t& path, const allocator& alloc) { return load(path, load_option::binary_read, alloc); }


// _____________________________________________________________________________
// async file operations

// Similar to load, this will load the complete file into an allocated buffer but
// the open, allocate, read and close occurs in another thread as well as the 
// on_complete so ensure its implementation is thread safe. The blob
// can either be moved in the on_complete or left alone where the system will
// auto-free it when the _OnCompletion goes out of scope. In success cases out_err 
// will be nullptr, but if it is valid then buffer is invalid.
void load_async(const path_t& path, completion_fn on_complete, void* user
								, load_option opt = load_option::binary_read
								, const allocator& alloc = default_allocator);

// Similar to save, this will save the specified buffer call on_complete. The open
// write and close will occur on another thread so ensure on_complete's implementation
// is thread safe. The blob must be moved initially, but will resurface
// in on_complete at which point the application can grab it again or allow it to 
// go out of scope and free itself. In success cases out_err is nullptr but if it is 
// valid then buffer is invalid.
void save_async(const path_t& path, blob&& buffer
								, completion_fn on_complete, void* user
								, save_option opt = save_option::binary_write);

// Waits until all async operations have completed
void wait();
bool wait_for(uint32_t timeout_ms);

// returns true if IO threads are running
bool joinable();

// waits for all associated IO operations to complete then joins all IO threads. If load_async is used
// this must be called before the application ends.
void join();

}}
