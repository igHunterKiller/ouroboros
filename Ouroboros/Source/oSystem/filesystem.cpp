// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oCore/finally.h>
#include <oCore/stringf.h>
#include <oSystem/filesystem.h>
#include <oSystem/process.h>
#include <oSystem/system.h>
#include <oSystem/windows/win_error.h>
#include <oSystem/windows/win_iocp.h>
#include <oBase/date.h>
#include <oString/string.h>
#include <oMemory/memory.h>

#include <io.h>
#include <memory>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Shlobj.h>

// Use these instead of oTHROW_SYSERR for the filesystem category
#define oERRC(err_code) ouro::filesystem::make_error_code(std::errc::err_code)
#define oFSTHROW_SYSERR0(err_code) throw std::system_error(oERRC(err_code))
#define oFSTHROW_SYSERR(err_code, fmt, ...) throw std::system_error(oERRC(err_code), ouro::stringf(fmt, ## __VA_ARGS__))

namespace ouro {

template<> const char* as_string(const filesystem::file_type& type)
{
	static const char* s_names[] = 
	{
		"block_file",
		"character_file",
		"directory_file",
		"fifo_file",
		"file_not_found",
		"regular_file",
		"socket_file",
		"status_unknown",
		"symlink_file",
		"type_unknown",
		"read_only_directory_file",
		"read_only_file",
	};
	return as_string(type, s_names);
}

}

#define oFSTHROW0(err_code) throw filesystem_error(oERRC(err_code))
#define oFSTHROW01(err_code, path1) throw filesystem_error(path1, oERRC(err_code))
#define oFSTHROW02(err_code, path1, path2) throw filesystem_error(path1, path2, oERRC(err_code))

#define oFSTHROW(err_code, format, ...) do \
{	lstring msg; vsnprintf(msg, format, # __VA_ARGS__ ); \
	throw filesystem_error(msg, oERRC(err_code)); \
} while (false)

#define oFSTHROW1(err_code, path1, format, ...) do \
{	lstring msg; vsnprintf(msg, format, # __VA_ARGS__ ); \
	throw filesystem_error(msg, path1, oERRC(err_code)); \
} while (false)

#define oFSTHROW2(err_code, path1, path2, format, ...) do \
{	lstring msg; vsnprintf(msg, format, # __VA_ARGS__ ); \
	throw filesystem_error(msg, path1, path2, oERRC(err_code)); \
} while (false)

#define oFSTHROWLAST() throw windows::error()
#define oFSTHROWLAST1(path1) throw windows::error()
#define oFSTHROWLAST2(path1, path2) throw windows::error()

#define oFSTHROW_FOPEN(err, path) do \
{	char strerr[256]; \
	strerror_s(strerr, err); \
	throw filesystem_error(strerr, path, ouro::filesystem::make_error_code((std::errc)err)); \
} while (false)

#define oFSTHROW_FOPEN0() do \
{	errno_t err = 0; \
	_get_errno(&err); \
	char strerr[256]; \
	strerror_s(strerr, err); \
	throw filesystem_error(strerr, ouro::filesystem::make_error_code((std::errc)err)); \
} while (false)

namespace ouro { namespace filesystem { namespace detail {

class filesystem_category_impl : public std::error_category
{
public:
	const char* name() const noexcept override { return "filesystem"; }
	std::string message(int err_code) const override
	{
		return std::generic_category().message(err_code);
	}
};

}

const std::error_category& filesystem_category()
{
	static detail::filesystem_category_impl sSingleton;
	return sSingleton;
}

static bool is_dot(const char* filename)
{
	return (!strcmp("..", filename) || !strcmp(".", filename));
}

path_t app_path(bool include_filename)
{
	char Path[MAX_PATH];
	DWORD len = GetModuleFileNameA(GetModuleHandle(nullptr), Path, countof(Path));
	if (!len)
	{
		oCheck(GetLastError() != ERROR_INSUFFICIENT_BUFFER, std::errc::no_buffer_space, "");
		oThrow(std::errc::operation_not_permitted, "");
	}

	if (!include_filename)
	{
		char* p = Path + len - 1;
		while (*p != '\\' && p >= Path)
			p--;
		oCheck(p >= Path, std::errc::operation_not_permitted, "");
		*(++p) = '\0';
	}

	return Path;
}

path_t temp_path(bool include_filename)
{
	char Path[MAX_PATH+1];
	oCheck(GetTempPathA(countof(Path), Path), std::errc::operation_not_permitted, "");
	oVB(!include_filename || !GetTempFileNameA(Path, "tmp", 0, Path));
	return Path;
}

path_t log_path(bool include_filename, const char* exe_suffix)
{
	path_t Path = app_path(true);
	path_t Basename = Path.basename();
	Path.replace_filename();
	Path /= "Logs/";

	if (include_filename)
	{
		time_t theTime;
		time(&theTime);
		tm t;
		localtime_s(&t, &theTime);

		sstring StrTime;
		::strftime(StrTime, StrTime.capacity(), "%Y%m%d%H%M%S", &t);

		char StrLogFilename[oMAX_PATH];
		snprintf(StrLogFilename, "%s-%s-%u%s%s.txt"
			, Basename.c_str()
			, StrTime.c_str()
			, this_process::get_id()
			, exe_suffix ? "-" : ""
			, exe_suffix ? exe_suffix : "");

		Path.replace_filename(StrLogFilename);
	}

	return Path;
}

path_t desktop_path()
{
	char Path[MAX_PATH];
	oCheck(SHGetSpecialFolderPathA(nullptr, Path, CSIDL_DESKTOPDIRECTORY/*CSIDL_COMMON_DESKTOPDIRECTORY*/, FALSE), std::errc::operation_not_permitted, "");
	oCheck(strlcat(Path, "\\") < countof(Path), std::errc::no_buffer_space, "");
	return Path;
}

path_t system_path()
{
	char Path[MAX_PATH];
	UINT len = GetSystemDirectoryA(Path, countof(Path));
	oCheck(len, std::errc::operation_not_permitted, "");
	oCheck(len <= (MAX_PATH-2), std::errc::no_buffer_space, "");
	Path[len] = '\\';
	Path[len+1] = '\0';
	return Path;
}

path_t os_path()
{
	char Path[MAX_PATH];
	UINT len = GetWindowsDirectoryA(Path, countof(Path));
	oCheck(len, std::errc::operation_not_permitted, "");
	oCheck(len <= (MAX_PATH-2), std::errc::no_buffer_space, "");
	Path[len] = '\\';
	Path[len+1] = '\0';
	return Path;
}

path_t dev_path()
{
	path_t Root = app_path();
	while (1)
	{
		path_t leaf = Root.filename();
		if (!_stricmp("bin", leaf))
			return Root.remove_filename();
		Root.remove_filename();
		oCheck(!Root.empty(), std::errc::no_such_file_or_directory, "");
	} while (!Root.empty());
}

path_t data_path()
{
	path_t Data;
	try { Data = dev_path(); }
	catch (filesystem_error&) { Data = app_path(); }
	Data /= "data/";

	return Data;
}

path_t current_path()
{
	char Path[MAX_PATH];
	oCheck(GetCurrentDirectoryA(countof(Path), Path), std::errc::operation_not_permitted, "");
	
	return Path;
}

void current_path(const path_t& path)
{
	oVB_MSG(SetCurrentDirectory(path), "Path: %s", path.c_str());
		oFSTHROWLAST1(path);
}

path_t root_path(const char* root_name)
{
	if (!_stricmp("data", root_name)) return data_path();
	if (!_stricmp("app", root_name)) return app_path();
	if (!_stricmp("current", root_name)) return current_path();
	if (!_stricmp("desktop", root_name)) return desktop_path();
	if (!_stricmp("temp", root_name)) return temp_path();
	if (!_stricmp("dev", root_name)) return dev_path();
	if (!_stricmp("system", root_name)) return system_path();
	if (!_stricmp("os", root_name)) return os_path();
	if (!_stricmp("log", root_name)) return log_path();
	return "";
}

path_t resolve(const path_t& relative_path)
{
	#define APPEND() do { \
		absolute_path /= relative_path; \
    if (exists(absolute_path)) return absolute_path; } while(false)

	path_t absolute_path = app_path(); APPEND();
	absolute_path = current_path(); APPEND();
	absolute_path = system_path(); APPEND();
	absolute_path = os_path(); APPEND();
	absolute_path = desktop_path(); APPEND();
	absolute_path = data_path(); APPEND();
	absolute_path = temp_path(); APPEND();

	char PATH[4096];
	if (system::getenv(PATH, "PATH"))
	{
		char result[512];
		path_t CWD = current_path();
		if (search_path(result, PATH, relative_path.c_str(), CWD.c_str(), [&](const char* path)->bool { return exists(path); }))
			return result;
	}

	oFSTHROW01(no_such_file_or_directory, relative_path);
}

static file_status status_internal(DWORD dwFileAttributes)
{
	if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
		return file_status(file_type::file_not_found);

	if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
			return file_status(file_type::read_only_directory_file);
		return file_status(file_type::directory_file);
	}
	
	if (dwFileAttributes & FILE_ATTRIBUTE_READONLY)
		return file_status(file_type::read_only_file);
	
	// todo: add determination if a symlink_file
	// what are block/character file-s on Windows?

	return file_status(file_type::regular_file);
}

file_status status(const path_t& path)
{
	return status_internal(GetFileAttributesA(path));
}

bool exists(const path_t& path)
{
	return INVALID_FILE_ATTRIBUTES != GetFileAttributesA(path);
}

void read_only(const path_t& path, bool read_only)
{
	DWORD dwFileAttributes = GetFileAttributesA(path);
	if (dwFileAttributes == INVALID_FILE_ATTRIBUTES)
		oFSTHROW01(no_such_file_or_directory, path);

	if (read_only) dwFileAttributes |= FILE_ATTRIBUTE_READONLY;
	else dwFileAttributes &=~ FILE_ATTRIBUTE_READONLY;

	if (!SetFileAttributesA(path, dwFileAttributes))
		oFSTHROWLAST1(path);
}

template<typename WIN32_TYPE> static uint64_t file_size_internal(const WIN32_TYPE& file_data)
{
	return (file_data.nFileSizeHigh * (static_cast<uint64_t>(MAXDWORD) + 1)) 
		+ file_data.nFileSizeLow;
}

uint64_t file_size(const path_t& path)
{
	WIN32_FILE_ATTRIBUTE_DATA fd;
	if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fd))
		oFSTHROWLAST1(path);
	return file_size_internal(fd);
}

time_t last_write_time(const path_t& path)
{
	WIN32_FILE_ATTRIBUTE_DATA fad;
	if (0 == GetFileAttributesExA(path, GetFileExInfoStandard, &fad))
		oFSTHROWLAST1(path);
	return date_cast<time_t>(fad.ftLastWriteTime);
}

void last_write_time(const path_t& path, time_t _time)
{
	HANDLE hFile = CreateFileA(path, FILE_WRITE_ATTRIBUTES, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		oFSTHROWLAST1(path);
	oFinally { CloseHandle(hFile); };
	FILETIME time = date_cast<FILETIME>(_time);
	if (!SetFileTime(hFile, nullptr, nullptr, &time))
		oFSTHROWLAST1(path);
}

bool remove_filename(const path_t& path)
{
	return !!DeleteFileA(path);
}

bool remove_directory(const path_t& path)
{
	int Retried = 0;
Retry:

	if (!RemoveDirectory(path))
	{
		HRESULT hr = GetLastError();
		switch (hr)
		{
			case ERROR_DIR_NOT_EMPTY:
			{
				// if a file explorer is open, this may be a false positive
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				if (Retried > 1)
					oFSTHROW01(directory_not_empty, path);
				Retried++;
				goto Retry;
			}
			case ERROR_SHARING_VIOLATION: oFSTHROW01(device_or_resource_busy, path);
			default: break;
		}
		oFSTHROWLAST1(path);
	}

	return true;
}

uint32_t remove_all(const path_t& path)
{
	if (is_directory(path))
	{
		path_t wildcard(path);
		wildcard /= "*";
		uint32_t removed = 0;
		enumerate(wildcard, [](const path_t& path, const file_status& status, uint64_t size, void* user)->bool
		{
			if (!is_directory(status))
			{
				*(uint32_t*)user += remove_filename(path) ? 1 : 0;
			}
			return true;
		}, &removed);

		enumerate(wildcard, [](const path_t& path, const file_status& status, uint64_t size, void* user)->bool
		{
			if (is_directory(status))
				*(uint32_t*)user += remove_all(path);
			return true;
		}, &removed);

		removed += remove_directory(path) ? 1 : 0;
		return removed;
	}

	return remove_filename(path) ? 1 : 0;
}

void copy_file(const path_t& from, const path_t& to, copy_option opt)
{
	if (!CopyFileA(from, to, opt == copy_option::fail_if_exists))
		oFSTHROWLAST2(from, to);
}

uint32_t copy_all(const path_t& from, const path_t& to, copy_option opt)
{
	if (is_directory(from))
	{
		uint32_t copied = 0;
		file_status ToStatus = status(to);

		if (!exists(ToStatus))
			create_directory(to);

		else if (!is_directory(ToStatus))
			oFSTHROW02(is_a_directory, from, to);

		path_t wildcard(from);
		wildcard /= "*";

		struct ctx_t
		{
			path_t full_to;
			uint32_t copied;
			copy_option opt;
		};

		ctx_t ctx;
		ctx.full_to = to / "*"; // put a "filename" so it can be replaced below
		ctx.copied = 0;
		ctx.opt = opt;

		enumerate(wildcard, [](const path_t& path, const file_status& status, uint64_t size, void* user)->bool
		{
			ctx_t* ctx = (ctx_t*)user;

			ctx->full_to.replace_filename(path.filename());

			if (is_directory(status))
			{
				ctx->copied += copy_all(path, ctx->full_to, ctx->opt);
			}

			else
			{
				copy_file(path, ctx->full_to, ctx->opt);
				ctx->copied++;
			}
			
			return true;
		}, &ctx);

		return ctx.copied;
	}
	copy_file(from, to, opt);
	return 1;
}

void rename(const path_t& from, const path_t& to, copy_option opt)
{
	if (!exists(from))
		oFSTHROW01(no_such_file_or_directory, from);

	if (opt == copy_option::overwrite_if_exists && exists(to))
		remove_all(to);

	MoveFileA(from, to);
}

bool create_directory(const path_t& path)
{
	if (!CreateDirectoryA(path, nullptr))
	{
		HRESULT hr = GetLastError();
		switch (hr)
		{
			case ERROR_ALREADY_EXISTS: return false;
			case ERROR_PATH_NOT_FOUND: oFSTHROW01(no_such_file_or_directory, path); break;
			default: oFSTHROWLAST();
		}
	}

	return true;
}

static bool create_directories_internal(const path_t& path)
{
	if (path.empty())
		return false;

	if (!CreateDirectoryA(path, nullptr))
	{
		HRESULT hr = GetLastError();
		switch (hr)
		{
			case ERROR_ALREADY_EXISTS: return false;
			case ERROR_PATH_NOT_FOUND:
			{
				path_t parent = path.parent_path();

				if (!create_directories_internal(parent))
					return false; // pass thru error message

				// Now try again
				return create_directory(path);
			}
			default: oFSTHROWLAST1(path);
		}
	}

	return true;
}

bool create_directories(const path_t& path)
{
	return create_directories_internal(path);
}

// This returns information for the volume containing path
space_info space(const path_t& path)
{
	space_info s;
	if (!GetDiskFreeSpaceEx(path, (PULARGE_INTEGER)&s.available, (PULARGE_INTEGER)&s.capacity, (PULARGE_INTEGER)&s.free))
		oFSTHROWLAST1(path);
	return s;
}

// Enumerates all entries matching wildcard_path
void enumerate(const path_t& wildcard_path, enumerate_fn enumerator, void* user)
{
	WIN32_FIND_DATA fd;
	HANDLE hFind = FindFirstFile(wildcard_path, &fd);
	oFinally { if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind); };

	if (hFind != INVALID_HANDLE_VALUE)
	{
		path_t resolved(wildcard_path);

		while (true)
		{
			if (!is_dot(fd.cFileName))
			{
				file_status status = status_internal(fd.dwFileAttributes);
				resolved.replace_filename(fd.cFileName);
				if (!enumerator(resolved, status, file_size_internal(fd), user))
					break;
			}

			if (!FindNextFile(hFind, &fd))
				break;
		}
	}
}

void enumerate_recursively(const path_t& wildcard_path, enumerate_fn enumerator, void* user)
{
	struct ctx_t
	{
		const path_t* wildcard_path;
		enumerate_fn enumerator;
		void* user;
	};

	ctx_t ctx;
	ctx.wildcard_path = &wildcard_path;
	ctx.enumerator = enumerator;
	ctx.user = user;

	enumerate(wildcard_path, [](const path_t& path, const file_status& status, uint64_t size, void* user)->bool
	{
		ctx_t* ctx = (ctx_t*)user;

		if (is_directory(status))
		{
			path_t wildcard = path / ctx->wildcard_path->filename();
			enumerate_recursively(wildcard, ctx->enumerator, ctx->user);
			return true;
		}

		return ctx->enumerator(path, status, size, ctx->user);
	}, user);
}

void* map(const path_t& path, map_option opt, uint64_t offset, uint64_t size)
{
	HANDLE hFile = CreateFileA(path, opt == map_option::binary_read ? GENERIC_READ : (GENERIC_READ|GENERIC_WRITE), 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		oFSTHROWLAST();
	oFinally { CloseHandle(hFile); };

	SYSTEM_INFO si;
	GetSystemInfo(&si);
	byte_swizzle64 aligned_offset;
	aligned_offset.asullong = align_down(offset, si.dwAllocationGranularity);
	uint64_t offset_padding = offset - aligned_offset.asullong;
	uint64_t alignedSize = size + offset_padding;

	DWORD fProtect = opt == map_option::binary_read ? PAGE_READONLY : PAGE_READWRITE;
	HANDLE hMapped = CreateFileMapping(hFile, nullptr, fProtect, 0, 0, nullptr);
	if (!hMapped)
		oFSTHROWLAST();
	oFinally { CloseHandle(hMapped); };

	void* p = MapViewOfFile(hMapped, fProtect == PAGE_READONLY ? FILE_MAP_READ : FILE_MAP_WRITE, aligned_offset.asuint32[1], aligned_offset.asuint32[0], SIZE_T(alignedSize));
	if (!p)
		oFSTHROWLAST();

	// Exit with a ref count of 1 on the underlying HANDLE, held by MapViewOfFile
	// so the file is fully closed when unmap is called. Right now the count is at 
	// 3, but the finallies will remove 2.
	return (uint8_t*)p + offset_padding;
}

void unmap(void* mapped_ptr)
{
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	void* p = align_down(mapped_ptr, si.dwAllocationGranularity);
	if (!UnmapViewOfFile(p))
		oFSTHROWLAST();
}

native_file_handle get_native(file_handle hfile)
{
	return (native_file_handle)_get_osfhandle(_fileno(hfile));
}

file_handle open(const path_t& path, open_option opt)
{
	static char* opts = "rwa";

	char open[3];
	open[0] = opts[(int)opt % 3];
	open[1] = opt >= open_option::text_read ? 't' : 'b';
	open[2] = '\0';

	_set_errno(0);
	file_handle hfile = (file_handle)_fsopen(path, open, _SH_DENYNO);
	if (!hfile)
	{
		errno_t err = 0;
		_get_errno(&err);
		oFSTHROW_FOPEN(err, path);
	}
	return hfile;
}

void close(file_handle hfile)
{
	if (!hfile)
		oFSTHROW_SYSERR0(invalid_argument);
	if (0 != fclose((FILE*)hfile))
		oFSTHROW_FOPEN0();
}

bool at_end(file_handle hfile)
{
	return !!feof((FILE*)hfile);
}

path_t get_path(file_handle hfile)
{
	#if WINVER < 0x0600 // NTDDI_VISTA
		#error this function is implemented only for Windows Vista and later.
	#endif
	HANDLE f = get_native(hfile);
	path_string str;
	oVB(GetFinalPathNameByHandleA(f, str.c_str(), static_cast<DWORD>(str.capacity()), 0));
	return path_t(str);
}

void last_write_time(file_handle hfile, time_t time)
{
	HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(hfile));
	oCheck(hFile != INVALID_HANDLE_VALUE, std::errc::no_such_file_or_directory, "");
	FILETIME ftime = date_cast<FILETIME>(time);
	oVB(SetFileTime(hFile, 0, 0, &ftime));
}

void seek(file_handle hfile, int64_t offset, seek_origin origin)
{ 
	if (-1 == _fseeki64((FILE*)hfile, offset, (int)origin))
		oFSTHROW_FOPEN0();
}

uint64_t file_size(file_handle hfile)
{
	HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(hfile));
	oCheck(hFile != INVALID_HANDLE_VALUE, std::errc::no_such_file_or_directory, "");
	
	#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
		FILE_STANDARD_INFO fsi;
		oVB(GetFileInformationByHandleEx(hFile, FileStandardInfo, &fsi, sizeof(fsi)));
		return fsi.EndOfFile.QuadPart;
	#else
		LARGE_INTEGER fsize;
		oVB(GetFileSizeEx(hFile, &fsize));
		return fsize.QuadPart;
	#endif
}

uint64_t tell(file_handle hfile)
{
	uint64_t offset = _ftelli64((FILE*)hfile);
	if (offset == 1L)
		oFSTHROW_FOPEN0();
	return offset;
}

uint64_t read(file_handle hfile, void* dst, uint64_t dst_size, uint64_t read_size)
{
	const size_t ReadSize = (size_t)read_size;
	size_t bytesRead = fread_s(dst, (size_t)dst_size, 1, ReadSize, (FILE*)hfile);
	if (ReadSize != bytesRead && !at_end(hfile))
		oFSTHROW_FOPEN0();
	return bytesRead;
}

uint64_t write(file_handle hfile, const void* src, uint64_t src_size, bool flush)
{
	size_t bytesWritten = fwrite(src, 1, static_cast<size_t>(src_size), (FILE*)hfile);
	if ((size_t)src_size != bytesWritten)
		oFSTHROW_FOPEN0();
	if (flush)
		fflush((FILE*)hfile);
	return bytesWritten;
}

void save(const path_t& path, const void* src, size_t src_size, save_option opt)
{
	open_option OpenOption = open_option::binary_read;
	switch (opt)
	{
		case save_option::text_write: OpenOption = open_option::text_write; break;
		case save_option::text_append: OpenOption = open_option::text_append; break;
		case save_option::binary_write: OpenOption = open_option::binary_write; break;
		case save_option::binary_append: OpenOption = open_option::binary_append; break;
	}
	
	create_directories(path.parent_path());
	file_handle f = open(path, OpenOption);
	oFinally { close(f); };
	write(f, src, src_size);
}

void delete_buffer(char* ptr)
{
	delete [] ptr;
}

blob load(const path_t& path, load_option opt, const allocator& alloc)
{
	uint64_t FileSize = file_size(path);
	// in case we need a UTF32 nul terminator
	size_t AllocSize = size_t(FileSize + (opt == load_option::text_read ? 4 : 0));
	blob p = alloc.scoped_allocate(AllocSize);

	// put enough nul terminators for a UTF32 or 16 or 8
	if (opt == load_option::text_read)
		*(int*)&(((char*)p)[FileSize]) = 0;

	{
		file_handle f = open(path, opt == load_option::text_read ? open_option::text_read : open_option::binary_read);
		oFinally { close(f); };
		if (FileSize != read(f, p, AllocSize, FileSize) && opt == load_option::binary_read)
			oThrow(std::errc::io_error, "read failed: %s", path.c_str());
	}

	// record honest size (tools like FXC crash if any larger size is given)
	size_t HonestSize = (size_t)FileSize;

	if (opt == load_option::text_read)
	{
		utf_type type = utfcmp(p, __min(HonestSize, 512));
		switch (type)
		{
			case utf_type::utf32be: case utf_type::utf32le: HonestSize += 4; break;
			case utf_type::utf16be: case utf_type::utf16le: HonestSize += 2; break;
			case utf_type::ascii: HonestSize += 1; break;
		}
	}

	return blob(p.release(), HonestSize, p.deleter());
}

struct iocp_file
{
	iocp_file(const path_t& path, const allocator& alloc, completion_fn on_complete, void* user)
		: overlapped(nullptr)
		, handle(INVALID_HANDLE_VALUE)
		, file_size(0)
		, file_path(path)
		, allocator(alloc)
		, error_code(0)
		, open_type(open_read)
		, completion(on_complete)
		, user(user)
	{}

	enum open
	{
		open_read,
		open_read_unbuffered,
		open_read_text,
		open_read_unbuffered_text,
		open_write,
	};

	blob buffer;
	OVERLAPPED* overlapped;
	HANDLE handle;
	uint32_t file_size;
	path_t file_path;
	allocator allocator;
	long error_code;
	open open_type;
	completion_fn completion;
	void* user;
};

static void iocp_close(iocp_file* f, uint64_t unused = 0)
{
	if (!f->error_code && (f->open_type == iocp_file::open_read_text || f->open_type == iocp_file::open_read_unbuffered_text))
	{
		char* p = (char*)f->buffer;
		*(int*)&(p[f->file_size]) = 0;
		// the above implies support for UTF32, but this replace does not
		replace(p, f->file_size + 4, p, "\r\n", "\n");
	}

	if (f->completion)
	{
		if (!f->error_code)
		{
			DWORD dwNumBytes = 0;
			if (!GetOverlappedResult(f->handle, f->overlapped, &dwNumBytes, FALSE))
				f->error_code = GetLastError();
		}

		if (f->error_code)
		{
			windows::error winerr(f->error_code);
			oAssert(f->file_size == f->buffer.size(), "size mismatch");
			f->completion(std::ref(f->file_path), std::ref(f->buffer), &winerr, f->user);
		}
		else
			f->completion(std::ref(f->file_path), std::ref(f->buffer), nullptr, f->user);
	}

	if (f->handle != INVALID_HANDLE_VALUE)
	{
		f->error_code = CloseHandle(f->handle) ? 0 : GetLastError();
		f->handle = INVALID_HANDLE_VALUE;
	}

	f->file_size = 0;
	f->completion = nullptr;
	if (f->overlapped)
		windows::iocp::disassociate(f->overlapped);
	delete f;
}

static void iocp_open(iocp_file* f, iocp_file::open op)
{
	f->overlapped = nullptr;
	f->open_type = op;
	f->handle = CreateFile(f->file_path
		, op == iocp_file::open_write ? GENERIC_WRITE : GENERIC_READ
		, FILE_SHARE_READ
		, nullptr
		, op == iocp_file::open_write ? CREATE_ALWAYS : OPEN_EXISTING
		, FILE_FLAG_OVERLAPPED | ((f->open_type == iocp_file::open_read_unbuffered || f->open_type == iocp_file::open_read_unbuffered_text) ? FILE_FLAG_NO_BUFFERING : 0)
		, nullptr);
	if (f->handle != INVALID_HANDLE_VALUE)
	{
		f->error_code = 0;
		DWORD hi = 0;
		f->file_size = GetFileSize(f->handle, &hi);
		if (sizeof(f->file_size) > sizeof(DWORD))
			f->file_size |= ((uint64_t)hi << 32);
	}
	else
	{
		f->error_code = GetLastError();
		iocp_close(f);
	}
}

static uint32_t iocp_allocation_size(const iocp_file* f)
{
	uint32_t size = f->file_size;
	if (f->open_type == iocp_file::open_read_text || f->open_type == iocp_file::open_read_unbuffered_text)
		size += 4; // worst-case nul terminator for Unicode 32

	if (f->open_type == iocp_file::open_read_unbuffered || f->open_type == iocp_file::open_read_unbuffered_text)
		size = align(size, 4096);

	return size;
}

static uint32_t iocp_io_size(const iocp_file* f)
{
	uint32_t size = f->file_size;

	if (f->open_type == iocp_file::open_read_unbuffered || f->open_type == iocp_file::open_read_unbuffered_text)
		size = align(size, 4096);

	return size;
}

static void iocp_read(iocp_file* f, uint32_t offset = 0, uint32_t read_size = ~0u)
{
	f->overlapped = windows::iocp::associate(f->handle, iocp_close, f);
	f->overlapped->Offset = (DWORD)offset;
	f->overlapped->OffsetHigh = 0;
	if (read_size == ~0u)
		read_size = iocp_io_size(f);
	if (!ReadFile(f->handle, f->buffer, (uint32_t)read_size, nullptr, f->overlapped))
	{
		int errc = GetLastError();
		if (errc != ERROR_IO_PENDING)
		{
			iocp_close(f);
			f->error_code = errc;
		}
	}
}

static void iocp_write(iocp_file* f, uint32_t offset = ~0u, uint32_t src_size = ~0u)
{
	f->overlapped = windows::iocp::associate(f->handle, iocp_close, f);
	f->overlapped->Offset = (DWORD)offset;
	f->overlapped->OffsetHigh = 0;

	if (src_size == ~0u)
		src_size = (uint32_t)f->buffer.size();
	
	if (!WriteFile(f->handle, f->buffer, src_size, nullptr, f->overlapped))
	{
		int errc = GetLastError();
		if (errc != ERROR_IO_PENDING)
		{
			iocp_close(f);
			f->error_code = errc;
		}
	}
}

static void iocp_open_and_read_shared(iocp_file::open _OpenType, void* _pContext, uint64_t unused = 0)
{
	iocp_file* f = (iocp_file*)_pContext;

	iocp_open(f, _OpenType);
	if (!f->error_code)
	{
		f->buffer = std::move(f->allocator.scoped_allocate(iocp_allocation_size(f)));
		if (f->buffer)
			iocp_read(f);			
		else
			iocp_close(f);
	}
}

static void iocp_open_and_read(void* _pContext, uint64_t unused = 0)
{
	iocp_open_and_read_shared(iocp_file::open_read, _pContext, unused);
}

static void iocp_open_and_read_text(void* _pContext, uint64_t unused = 0)
{
	iocp_open_and_read_shared(iocp_file::open_read_text, _pContext, unused);
}

static void iocp_open_and_write(void* _pContext, uint64_t unused = 0)
{
	iocp_file* f = (iocp_file*)_pContext;
	if (f->buffer)
	{
		iocp_open(f, iocp_file::open_write);
		if (!f->error_code)
			iocp_write(f, 0);
	}
}

static void iocp_open_and_append(void* _pContext, uint64_t unused = 0)
{
	iocp_file* f = (iocp_file*)_pContext;
	if (f->buffer)
	{
		iocp_open(f, iocp_file::open_write);
		if (!f->error_code)
			iocp_write(f);
	}
}

void load_async(const path_t& path, completion_fn on_complete, void* user, load_option opt, const allocator& alloc)
{
	iocp_file* r = new iocp_file(path, alloc, on_complete, user);
	windows::iocp::post(opt == load_option::binary_read ? iocp_open_and_read : iocp_open_and_read_text, r);
}

void save_async(const path_t& path, blob&& buffer, completion_fn on_complete, void* user, save_option opt)
{
	if (opt != save_option::binary_write && opt != save_option::binary_append)
		oThrow(std::errc::invalid_argument, "only binary_write and binary_append is currently supported");

	iocp_file* r = new iocp_file(path, allocator(), on_complete, user);
	r->buffer = std::move(buffer);
	windows::iocp::post(opt == save_option::binary_write ? iocp_open_and_write : iocp_open_and_append, r);
}

void wait()
{
	windows::iocp::wait();
}

bool wait_for(uint32_t timeout_ms)
{
	return windows::iocp::wait_for(timeout_ms);
}

bool joinable()
{
	return windows::iocp::joinable();
}

void join()
{
	oCheck(joinable(), std::errc::invalid_argument, "");
	if (windows::iocp::joinable())
		windows::iocp::join();
}

}}
