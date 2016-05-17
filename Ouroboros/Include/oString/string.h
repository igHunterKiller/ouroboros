// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// string manipulation functions that really should be in the C/C++ standard

#pragma once
#include <oCore/restrict.h>
#include <oCore/snprintf.h>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <ctype.h>

#define oNEWLINE "\r\n"
#define oWHITESPACE " \t\v\f" oNEWLINE

#define oWNEWLINE L"\r\n"
#define oWWHITESPACE L" \t\v\f" oWNEWLINE

#define oDIGIT "0123456789"
#define oDIGIT_UNSIGNED oDIGIT "+"
#define oDIGIT_SIGNED oDIGIT "+-"

// _____________________________________________________________________________
// The most-standard (but not standard) secure strcpy/strcats.

size_t strlcat(char* dst, const char* src, size_t dst_num_chars);
size_t strlcpy(char* dst, const char* src, size_t dst_num_chars);
size_t wcslcat(wchar_t* dst, const wchar_t* src, size_t dst_num_chars);
size_t wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_num_chars);
size_t mbsltowsc(wchar_t* dst, const char* src, size_t dst_num_chars);
size_t wcsltombs(char* dst, const wchar_t* src, size_t dst_num_chars);

template<size_t num_chars> size_t strlcat(char (&dst)[num_chars], const char* src) { return strlcat(dst, src, num_chars); }
template<size_t num_chars> size_t strlcpy(char (&dst)[num_chars], const char* src) { return strlcpy(dst, src, num_chars); }
template<size_t num_chars> size_t wcslcat(wchar_t (&dst)[num_chars], const wchar_t* src) { return wcslcat(dst, src, num_chars); }
template<size_t num_chars> size_t wcslcpy(wchar_t (&dst)[num_chars], const wchar_t* src) { return wcslcpy(dst, src, num_chars); }
template<size_t num_chars> size_t mbsltowcs(wchar_t (&dst)[num_chars], const char* src) { return mbsltowcs(dst, src, num_chars); }
template<size_t num_chars> size_t wcsltombs(char (&dst)[num_chars], const wchar_t* src) { return wcsltombs(dst, src, num_chars); }

namespace ouro {

// _____________________________________________________________________________
// Copies that MSVC complains about

inline char* strncpy(char* dst, size_t dst_size, const char* src, size_t num_chars) { return strncpy_s(dst, dst_size, src, num_chars) ? nullptr : dst; }
inline wchar_t* wcsncpy(wchar_t* dst, size_t dst_size, const wchar_t* src, size_t num_chars) { return wcsncpy_s(dst, dst_size, src, num_chars) ? nullptr : dst; }

template<size_t size> char* strncpy(char (&dst)[size], const char* src, size_t num_chars) { return strncpy(dst, size, src, num_chars); }
template<size_t size> char* wcsncpy(wchar_t (&dst)[size], const wchar_t* src, size_t num_chars) { return wcsncpy(dst, size, src, num_chars); }

// _____________________________________________________________________________
// Formatting

// tolower doesn't work on wchar_ts and std::to_lower has annoying locale required
inline char to_lower(char c) { return (char)tolower(c); }
inline char to_upper(char c) { return (char)toupper(c); }
wchar_t to_lower(wchar_t wc);
wchar_t to_upper(wchar_t wc);

template<typename charT> void to_lower(charT* str) { while (*str) *str++ = to_lower(*str); }
template<typename charT> void to_upper(charT* str) { while (*str) *str++ = to_upper(*str); }
template<typename charT> void to_lower(charT* begin, charT* end) { while (begin < end) *begin++ = to_lower(*begin); }
template<typename charT> void to_upper(charT* begin, charT* end) { while (begin < end) *begin++ = to_upper(*begin); }

// Adds "...\0" to the end of the buffer - useful when recovering from string 
// overruns that can be truncated.
char* ellipsize(char* dst, size_t dst_size);
template<size_t size> char* ellipsize(char (&dst)[size]) { return ellipsize(dst, size); }

wchar_t* wcsellipsize(wchar_t* dst, size_t _NumChars);
template<size_t n> wchar_t* ellipsize(wchar_t (&dst)[n]) { return wcsellipsize(dst, n); }

// Fills the specified buffer with a size in days hours minutes seconds (and 
// milliseconds). This returns the length the string would be but writing stops
// at the end of dst_size. If length >= dst_size then
// the destination is not large enough to hold the nul-terminated string. This
// will not return -1 like snprintf does.
int format_duration(char* dst, size_t dst_size, double seconds, bool abbreviated = false, bool include_milliseconds = true);
template<size_t size> int format_duration(char (&dst)[size], double seconds, bool abbreviated = false, bool include_milliseconds = true) { return format_duration(dst, size, seconds, abbreviated, include_milliseconds); }

// Fills the specified buffer with a size in either bytes, KB, MB, GB, or TB 
// depending on the number of bytes specified. This returns the same result as 
// snprintf.
int format_bytes(char* dst, size_t dst_size, uint64_t bytes, size_t num_precision_digits);
template<size_t size> int format_bytes(char (&dst)[size], uint64_t bytes, size_t num_precision_digits) { return format_bytes(dst, size, bytes, num_precision_digits); }

// For numbers, this inserts commas where they ought to be (every 3 numbers)
// this returns nullptr on failure, else dst.
char* format_commas(char* dst, size_t dst_size, int number);
template<size_t size> char* format_commas(char (&dst)[size], int number) { return format_commas(dst, size, number); }
char* format_commas(char* dst, size_t dst_size, unsigned int number);
template<size_t size> char* format_commas(char (&dst)[size], unsigned int number) { return format_commas(dst, size, number); }

// Returns the appropriate suffix [st nd rd th] for a number
const char* ordinal(size_t number);

// does a formatted strcat. This returns the total new length of the string. If 
// that length is >= dst_size, then the destination is not big 
// enough to hold the concatenation. This never returns -1 like snprintf.
inline int vsncatf(char* dst, size_t dst_size, const char* fmt, va_list args)
{
	int len = static_cast<int>(strlen(dst ? dst : ""));
	int result = vsnprintf(dst + len, dst_size - len, fmt, args);
	if (result < -1 || static_cast<size_t>(result) > (dst_size - len))
		result = vsnprintf(nullptr, 0, fmt, args);
	return len + result;
}

inline int sncatf(char* dst, size_t dst_size, const char* fmt, ...)
{
	va_list args; va_start(args, fmt);
	int l = vsncatf(dst, dst_size, fmt, args);
	va_end(args);
	return l;
}

template<size_t size> int vsncatf(char* dst, size_t dst_size, const char* fmt, va_list args) { return vsncatf(dst, size, fmt, args): }
template<size_t size> int sncatf(char (&dst)[size], const char* fmt, ...)
{
	va_list args; va_start(args, fmt);
	int l = vsncatf(dst, size, fmt, args);
	va_end(args);
	return l;
}

// Returns the pointer into typeinfo_name that represents just the name of the 
// user type, thus skipping any prefix [enum|class|struct|union]. This does not
// behave well for built-in types.
const char* type_name(const char* typeinfo_name);

// _____________________________________________________________________________
// Whitespace

// Remove all chars found in to_trim from the beginning of the string. trimmed 
// can be the same as src. Returns trimmed.
char* trim_left(char* trimmed, size_t trimmed_size, const char* src, const char* to_trim);
template<size_t size> char* trim_left(char (&trimmed)[size], const char* src, const char* to_trim = oWHITESPACE) { return trim_left(trimmed, size, src, to_trim); }

// Remove all chars found in to_trim form the end of the string. trimmed can 
// be the same as strSource. Returns trimmed.
char* trim_right(char* trimmed, size_t trimmed_size, const char* src, const char* to_trim);
template<size_t size> char* trim_right(char (&trimmed)[size], const char* src, const char* to_trim = oWHITESPACE) { return trim_right(trimmed, size, src, to_trim); }

// Trims both the left and right side of a string
inline char* trim(char* trimmed, size_t trimmed_size, const char* src, const char* to_trim = oWHITESPACE) { return trim_right(trimmed, trimmed_size, trim_left(trimmed, trimmed_size, src, to_trim), to_trim); }
template<size_t size> char* trim(char (&trimmed)[size], const char* src, const char* to_trim = oWHITESPACE) { return trim(trimmed, size, src, to_trim); }

// Replaces any run of whitespace with a single ' ' character. Returns dst
char* clean_whitespace(char* dst, size_t dst_size, const char* src, char replacement = ' ', const char* to_prune = oWHITESPACE);
template<size_t size> char* clean_whitespace(char (&dst)[size], const char* src, char replacement = ' ', const char* to_prune = oWHITESPACE) { return clean_whitespace(dst, dst_size, src, replacement, to_prune); }

// _____________________________________________________________________________
// Search

// like strstr, but case-insensitive
const char* strcasestr(const char* str, const char* substring);
char* strcasestr(char* str, const char* substring);

// make naming consistent
inline const char* stristr(const char* str, const char* substring) { return strcasestr(str, substring); }
inline char* stristr(char* str, const char* substring) { return strcasestr(str, substring); }

// like strstr but for a limited number of characters
const char* strnstr(const char* str, const char* substring, size_t len);
char* strnstr(char* str, const char* substring, size_t len);

// like strnstr but case-insensitive
const char* strncasestr(const char* str, const char* find, size_t len);
char* strncasestr(char* str, const char* find, size_t len);

// make naming consistent
inline const char* strnistr(const char* str, const char* find, size_t len) { return strncasestr(str, find, len); }
inline char* strnistr(char* str, const char* find, size_t len) { return strncasestr(str, find, len); }

// like strstr, but finds the last occurrence as if searching in reverse
const char* rstrstr(const char* str, const char* substring);
char* rstrstr(char* str, const char* substring);

// behaves like strcspn but starts are str1 and goes through the string in
// reverse until buffer_start is reached.
size_t rstrcspn(const char* buffer_start, const char* str1, const char* str2);
size_t rstrcspn(char* buffer_start, char* str1, const char* str2);

// Insert one string into another in-place. insertion_point must point into 
// src. If replacement_length is non-zero then that number of characters 
// from insertion_point on will be overwritten by the insertion. This returns
// the position immediately after the new insertion. If there isn't enough room
// in the buffer, then nullptr is returned.
char* insert(char* src, size_t src_size, char* insertion_point, size_t replacement_length, const char* insertion);
template<size_t size> char* insert(char (&src)[size], char* insertion_point, size_t replacement_length, const char* insertion)  { return insert(src, size, insertion_point, replacement_length, insertion); }

// replace all occurrences of find in src with replace and copy 
// the result to dst. 
errno_t replace(char* oRESTRICT result, size_t result_size, const char* oRESTRICT src, const char* oRESTRICT find, const char* oRESTRICT replace);
template<size_t size> errno_t replace(char (&result)[size], const char* oRESTRICT src, const char* oRESTRICT find, const char* oRESTRICT replace) { return replace(result, size, src, find, replace); }

// char version of above
errno_t replace(char* oRESTRICT result, size_t result_size, const char* oRESTRICT src, char chr_find, char replace);
template<size_t size> errno_t replace(char (&result)[size], const char* oRESTRICT src, char chr_find, char replace) { return replace(result, size, src, chr_find, replace); }

// first param must be pointing into a string at the open brace. From there this 
// will find the brace at the same level of recursion - internal pairs are 
// ignored. The 'brace' can be any single character. This will return nullptr
// if the braces are unbalanced.
const char* next_matching(const char* p_open_brace, char close_brace);
char* next_matching(char* p_open_brace, char close_brace);

// first param must be pointing into a string at the close brace. From there this
// will find the brace at the same level of recursion - internal pairs are ignored.
// The 'brace' can be any single character. This will return nullptr if the braces
// are unbalanced.
const char* prev_matching(const char* buffer_start, const char* p_close_brace, char open_brace);
char* prev_matching(char* buffer_start, char* p_close_brace, char open_brace);

// Same as the single-char next_matching but operates on braces that are multiple 
// characters. (c-style comments, #if #endif <!-- --> and other such delimiters).
const char* next_matching(const char* p_open_brace, const char* open_brace, const char* close_brace);
char* next_matching(char* p_open_brace, const char* open_brace, const char* close_brace);

// Same as the single-char prev_matching but operates on braces that are multiple 
// characters. (c-stle comments, #if #endif <!-- --> and other such delimiters).
const char* prev_matching(const char* buffer_start, const char* p_close_brace, const char* open_brace, const char* close_brace);
char* prev_matching(char* buffer_start, char* p_close_brace, const char* open_brace, const char* close_brace);

// copy token first for multi-threaded or non-destructive strtok. If the 
// parsing exits early end_strtok() must be called on ctx. If parsing
// exits due to a nullptr return value, cleanup need not be called.
char* strtok(const char* token, const char* delim, char** ctx);
void end_strtok(char** ctx);

// Posix form of a safer strtok
inline char* strtok_r(char* token, const char* delim, char** ctx) { return strtok_s(token, delim, ctx); }

// _____________________________________________________________________________
// Misc

// This function takes a single value (usually an enum) and returns a constant
// string representation of it.
typedef const char* (*asstr_fn)(const int& single_flag);

// Fill dst with a string representation of flags by going through
// each bit set and calling as_string on that value. If no bits are set, 
// dst will be set to all_zeros_value.
char* strbitmask(char* dst, size_t dst_size, int flags, const char* all_zeros_value, asstr_fn as_string);
template<size_t size> char* strbitmask(char (&dst)[size], int flags, const char* all_zeros_value, asstr_fn as_string) { return strbitmask(dst, size, flags, all_zeros_value, as_string); }
template<typename T> char* strbitmask(char* dst, size_t dst_size, int flags, const char* all_zeros_value, const char* (*as_string)(const T& value)) { return strbitmask(dst, dst_size, flags, all_zeros_value, (asstr_fn)as_string); }
template<typename T, size_t size> char* strbitmask(char (&dst)[size], int flags, const char* all_zeros_value, const char* (*as_string)(const T& value)) { return strbitmask(dst, size, flags, all_zeros_value, (asstr_fn)as_string); }

}
