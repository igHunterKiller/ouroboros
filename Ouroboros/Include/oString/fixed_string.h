// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once
#include <oString/string_traits.h>
#include <oCore/stringize.h>
#include <cstring>
#include <cwchar>
#include <stdexcept>

// Support oString APIs
#define TSTR_PARAMS typename charT, size_t Capacity
#define TSTR template<TSTR_PARAMS>
#define STRT fixed_string<charT, Capacity>
 
namespace ouro {

	template<typename charT, size_t size, typename traitsT = string_traits<charT>>
	class fixed_string
	{
	public:
		// until we can make capacity() a constexpr expose the template arg
		static const size_t Capacity = size;
		typedef traitsT traits;
		typedef typename traits::char_type char_type;
		typedef typename traits::size_type size_type;
		typedef char_type* iterator;
		typedef const char_type* const_iterator;
		typedef std::pair<const char_type*, const char_type*> string_piece_type;
		typedef char_type(&array_ref)[Capacity];
		typedef const char_type(&const_array_ref)[Capacity];

		fixed_string() { *s = 0; }
		fixed_string(const string_piece_type& str) { assign(str.first, str.second); }
		fixed_string(const char_type* start, const char_type* end) { assign(start, end); }
		fixed_string(const char_type* str) { if (traits::copy(s, str, Capacity) > Capacity) throw std::length_error("destination is not large enough"); }

		template<typename charU> fixed_string(const charU* str) { operator=(str); }
		template<typename charU, size_type _CapacityU> fixed_string(const fixed_string<charU, _CapacityU>& that) { operator=(that); }
		template<typename charU> const fixed_string& operator=(const charU* that) { if (string_traits<charU>::copy(s, that, Capacity) > Capacity) throw std::length_error("destination is not large enough"); return *this; }
		template<typename charU, size_type _CapacityU> const fixed_string& operator=(const fixed_string<charU, _CapacityU>& that) { if (string_traits<charU>::copy(s, that.c_str(), Capacity) > Capacity) throw std::length_error("destination is not large enough"); return *this; }

		iterator begin() { return s; }
		const_iterator begin() const { return s; }

		// NOTE: To keep class size to same-as-c-array, length is recalculated on
		// each call, so the typical for (auto it = begin(); it != end(); ++it) 
		// idiom may be costly. Factor out the call to end().
		iterator end() { return s + length(); }
		const_iterator end() const { return s + length(); }

		void clear() { *s = 0; }
		bool empty() const { return *s == 0; }
		size_type size() const { return traits::length(s); }
		size_type length() const { return size(); }
		/* constexpr */ size_type capacity() const { return Capacity; }

		size_type assign(const char_type* start, const char_type* end)
		{
			ptrdiff_t size = std::distance(start, end);
			if (size >= Capacity)
				throw std::length_error("destination is not large enough");
			traits::copy(s, start, size + 1);
			s[size] = 0;
			return size;
		}

		size_type assign(const char_type* str)
		{
			return traits::copy(s, str, Capacity);
		}

		size_type append(const char_type* str)
		{
			size_type size = traits::cat(s, str, Capacity);
			if (size > Capacity)
				throw std::length_error("destination is not large enough");
			return size;
		}

		size_type append(const char_type* start, const char_type* end)
		{
			size_type size = length();
			char_type* st = s + size;
			size += std::distance(start, end);
			if (size >= Capacity)
				throw std::length_error("destination is not large enough");
			traits::copy(st, start, Capacity);
			s[size] = 0;
			return size;
		}

		size_type append(const string_piece_type& str) { return append(str.first, str.second); }

		size_type append(char_type c) { char_type str[2] = { c, '\0'}; return append(str); }

		fixed_string& operator+=(const char_type* str) { append(str); return *this;  }
		fixed_string& operator+=(const string_piece_type& str) { append(str); return *this;  }
		fixed_string& operator+=(char_type c) { append(c); return *this;  }

		void copy_to(charT* dst, size_t dst_size) const
		{
			if (traits::copy(dst, s, dst_size) > dst_size)
				throw std::length_error("destination is not large enough");
		}

		array_ref c_str() { return s; }
		const_array_ref c_str() const { return s; }

		operator char_type*() { return s; }
		operator const char_type*() const { return s; }

		int compare(const char_type* that) const { return traits::compare(s, that); }
		int compare(const fixed_string& that) const { return compare(that.c_str()); }

		bool operator==(const char_type* that) const { return !compare(that); }
		bool operator==(const fixed_string& that) const { return !compare(that); }

		bool operator<(const fixed_string& that) const { return compare(that) < 0; }
		bool operator<(const char_type* that) const { return compare(that) < 0; }

	private:
		char_type s[Capacity];
	};

	typedef fixed_string<char, 64> sstring;
	typedef fixed_string<char, 128> mstring;
	typedef fixed_string<char, 512> lstring;
	typedef fixed_string<char, 2048> xlstring;
	typedef fixed_string<char, 4096> xxlstring;
	typedef lstring path_string;
	typedef lstring uri_string;

	typedef fixed_string<wchar_t, 64> swstring;
	typedef fixed_string<wchar_t, 128> mwstring;
	typedef fixed_string<wchar_t, 512> lwstring;
	typedef fixed_string<wchar_t, 2048> xlwstring;
	typedef fixed_string<wchar_t, 4096> xxlwstring;
	typedef lwstring path_wstring;
	typedef lwstring uri_wstring;

	TSTR int format_duration(STRT& dst, double _TimeInSeconds, bool _Abbreviated = false, bool _IncludeMS = true) { return format_duration(dst, Capacity, _TimeInSeconds, _Abbreviated, _IncludeMS); }
	TSTR int format_bytes(STRT& dst, unsigned long long bytes, size_t _NumPrecisionDigits) { return format_bytes(dst, dst.capacity(), bytes, _NumPrecisionDigits); }
	TSTR char* format_commas(STRT& dst, int _Number) { return format_commas(dst, dst.capacity(), _Number); }
	TSTR char* format_commas(STRT& dst, unsigned int _Number) { return format_commas(dst, dst.capacity(), _Number); }

	template<size_t capacity> char* ellipsize(fixed_string<char, capacity>& dst) { return ellipsize(dst.c_str(), dst.capacity()); }
	template<size_t capacity> wchar_t* ellipsize(fixed_string<wchar_t, capacity>& dst) { return wcsellipsize(dst.c_str(), dst.capacity()); }

	TSTR char* trim_left(STRT& trimmed, const char* src, const char* to_trim = oWHITESPACE) { return trim_left(trimmed, trimmed.capacity(), src, to_trim); }
	TSTR char* trim_right(STRT& trimmed, const char* src, const char* to_trim = oWHITESPACE) { return trim_right(trimmed, trimmed.capacity(), src, to_trim); }
	TSTR char* trim(STRT& trimmed, const char* src, const char* to_trim = oWHITESPACE) { return trim(trimmed, trimmed.capacity(), src, to_trim); }
	TSTR char* clean_whitespace(STRT& dst, const char* src, char replacement = ' ', const char* to_prune = oWHITESPACE) { return clean_whitespace(dst, dst.capacity(), src, replacement, to_prune); }

	TSTR char* insert(STRT& src, char* ins_point, size_t replacement_length, const char* ins)  { return insert(src, src.capacity(), ins_point, replacement_length, ins); }
	TSTR errno_t replace(STRT& str_result, const char* oRESTRICT src, const char* str_find, const char* str_replace) { return replace(str_result, str_result.capacity(), src, str_find, str_replace); }
	TSTR errno_t replace(STRT& str_result, const char* oRESTRICT src, char chr_find, char chr_replace) { return replace(str_result, str_result.capacity(), src, chr_find, chr_replace); }

	TSTR char* to_string(char* dst, size_t dst_size, const STRT& value)
	{
		try { value.copy_to(dst, dst_size); }
		catch (std::exception&) { return nullptr; }
		return dst;
	}

	TSTR bool from_string(STRT* _pValue, const char* src)
	{
		try { *_pValue = src; }
		catch (std::exception&) { return false; }
		return true;
	}

	template<TSTR_PARAMS, typename T> char* to_string(STRT& dst, const T& value) { return to_string(dst.c_str(), dst.capacity(), value); }

	TSTR void to_lower(STRT& _String) { to_lower<STRT::char_type>(_String.c_str()); }
	TSTR void to_upper(STRT& _String) { to_upper<STRT::char_type>(_String.c_str()); }
	
	TSTR int vsncatf(STRT& dst, const char* fmt, va_list args) { return vsncatf(dst, dst.capacity(), fmt, args); }
	TSTR int sncatf(STRT& dst, const char* fmt, ...) { va_list args; va_start(args, fmt); int l = vsncatf(dst, fmt, args); va_end(args); return l; }

	TSTR char* clean_path(STRT& dst, const char* src_path, char separator = '/', bool zero_buffer = false) { return clean_path(dst, dst.capacity(), src_path, separator, zero_buffer); }
	TSTR char* relativize_path(STRT& dst, const char* base_path, const char* full_path) { return relativize_path(dst, dst.capacity(), base_path, full_path); }

	TSTR int vsnprintf(STRT& dst, const char* fmt, va_list args) { return vsnprintf(dst, Capacity, fmt, args); }
	TSTR char* strncpy(STRT& dst, const char* src, size_t num_chars) { return strncpy(dst, dst.capacity(), src, num_chars); }
	TSTR char* wcsncpy(STRT& dst, const wchar_t* src, size_t num_chars) { return wcsncpy(dst, dst.capacity(), src, num_chars); }
}

TSTR int snprintf(ouro::STRT& dst, const char* fmt, ...) { va_list args; va_start(args, fmt); int l = ouro::vsnprintf(dst, fmt, args); va_end(args); return l; }
TSTR size_t strlcat(ouro::STRT& dst, const char* src) { return strlcat(dst, src, dst.capacity()); }
TSTR size_t strlcpy(ouro::STRT& dst, const char* src) { return strlcpy(dst, src, dst.capacity()); }
TSTR size_t wcslcat(ouro::STRT& dst, const wchar_t* src) { return wcslcat(dst, src, dst.capacity()); }
TSTR size_t wcslcpy(ouro::STRT& dst, const wchar_t* src) { return wcslcpy(dst, src, dst.capacity()); }
TSTR size_t mbsltowsc(ouro::STRT& dst, const char* src) { return mbsltowsc(dst, src, dst.capacity()); }
TSTR size_t wcsltombs(ouro::STRT& dst, const wchar_t* src) { return wcsltombs(dst, src, dst.capacity()); }

#undef STRT
#undef TSTR
#undef TSTR_PARAMS
