// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// policy system for converting types (particularly enums) to and from strings
// When defining an enum or other ubiquitous type, you are likely to want to 
// debug and/or serialize it to some text format eventually, so it is good form
// to implement as_string for each enum defined.

// implementation note: to avoid dependencies so as to keep this header very
// lightweight, snprintf is used for strcpy, or rather strlcpy/strcpy_s.
// Some other secure-CRT concessions are made. The goal should be to favor posix
// where the C++ standard fails so promote requirements to oCore like snprintf
// if necessary.

#pragma once
#include <oCore/countof.h>
#include <oCore/snprintf.h>
#include <cstdint>
#include <cstring>
#include <type_traits>

// _____________________________________________________________________________
// Default implementation macros. Use these to meet the requirements of the 
// policy API.

#define oDEFINE_TO_STRING(_T)                         template<> char* to_string(char* dst, size_t dst_size, const _T& value) { return ::ouro::default_to_string(dst, dst_size, value); }
#define oDEFINE_FROM_STRING2(_T, invalid_value)       template<> bool from_string(_T* out_value, const char* src) { return ::ouro::default_from_string<_T>(out_value, src, invalid_value); }
#define oDEFINE_FROM_STRING(_T)                       oDEFINE_FROM_STRING2(_T, _T(0))
#define oDEFINE_FLAGS_FROM_STRING2(_T, invalid_value) template<> bool from_string(_T* out_value, const char* src) { return ::ouro::default_from_string_bitflags<_T>(out_value, src, invalid_value); }
#define oDEFINE_FLAGS_FROM_STRING(_T)                 oDEFINE_FLAGS_FROM_STRING2(_T, _T(0))
#define oDEFINE_TO_FROM_STRING(_T)                    oDEFINE_TO_STRING(_T) oDEFINE_FROM_STRING(_T)

namespace ouro {

// _____________________________________________________________________________
// Primary 'policy' APIs: as_string to_string from_string
	
// returns a const string representation of the specified value, mostly for invariants like enums
template<typename T> const char* as_string(const T& value);

// returns dst on success, or nullptr
template<typename T>              char* to_string(char* dst, size_t dst_size, const T& value);
template<typename T, size_t size> char* to_string(char (&dst)[size], const T& value) { return to_string<T>(dst, size, value); }

// permutation of from_string for fixed-size string destination
                      inline char* to_string(char* dst, size_t dst_size, const char* src) { return -1 != snprintf(dst, dst_size, "%s", src ? src : "(null)") ? dst : nullptr; }
template<size_t size>        char* to_string(char (&dst)[size], const char* src) { return from_string(dst, size, src); }

// returns true if out_value receives a successful conversion of the string to type T
template<typename T> bool from_string(T* out_value, const char* src);

// permutation of from_string for fixed-size string destination
                      inline bool from_string(char* dst, size_t dst_size, const char* src) { return -1 != snprintf(dst, dst_size, "%s", src ? src : "(null)"); }
template<size_t size>        bool from_string(char (&dst)[size], const char* src) { return from_string(dst, size, src); }


// _____________________________________________________________________________
// Helper & default implementations

// simplify converting from an enum to another type. Use this to implement as_string for enum types
template<typename enumT, typename T, size_t size> T as_string(const enumT& e, T (&names)[size])
{
  static_assert(std::is_enum<enumT>::value, "not enum");
	match_array_e(names, enumT);
	size_t i = (size_t)e;
	return (i >= 0 && i <= (size_t)enumT::count) ? names[i] : T(0);
}

// copies as_string to destination
template<typename T> char* default_to_string(char* dst, size_t dst_size, const T& value) { return to_string(dst, dst_size, as_string(value)); }

// from_string for enums that are defined to act as bitflags
template<typename T> bool default_from_string_bitflags(T* out_value, const char* src, const typename std::underlying_type<T>::type& invalid_value)
{
  static_assert(std::is_enum<T>::value, "not enum");
	for (std::underlying_type<T>::type e = 1; e; e <<= 1)
  {
    if (!_stricmp(src, as_string(T(e))))
    {
      *out_value = T(e);
      return true;
    }
  }
  *out_value = T(invalid_value);
  return false;
}

namespace detail {

// from_string for enums
template<typename T> bool default_from_string(T* out_value, const char* src, const T& invalid_value, std::true_type)
{
  static_assert(std::is_enum<T>::value, "not enum");
  *out_value = invalid_value;
  for (std::underlying_type<T>::type e = 0; e < (std::underlying_type<T>::type)T::count; e++)
  {
    if (!_stricmp(src, as_string(T(e))))
    {
      *out_value = T(e);
      return true;
    }
  }
  return false;
}

// from_string for non-enums
template<typename T> bool default_from_string(T* out_value, const char* src, const T& invalid_value, std::false_type)
{
  bool success = ouro::from_string(out_value, src);
	if (!success) *out_value = invalid_value;
	return success;
}

}

// enforces the enum-has-count field policy if an enum, otherwise defaults to policy from_string (the one that requires user implementation)
template<typename T> bool default_from_string(T* out_value, const char* src, const T& invalid_value)
{
  return ouro::detail::default_from_string<T>(out_value, src, invalid_value, std::is_enum<T>::type());
}


// _____________________________________________________________________________
// Builtin specializations

template<> inline const char* as_string(const bool& value)                                    { return value ? "true" : "false"; }
template<> inline       char* to_string(char* dst, size_t dst_size, const char* const& value) { return -1 != snprintf(dst, dst_size, "%s", value ? value : "(null)") ? dst : nullptr; }
template<> inline       char* to_string(char* dst, size_t dst_size, const bool& value)        { return -1 != snprintf(dst, dst_size, "%s", value ? "true" : "false") ? dst : nullptr; }
template<> inline       char* to_string(char* dst, size_t dst_size, const char& value)        { if (!dst || dst_size < 2) return nullptr; dst[0] = value; dst[1] = '\0'; return dst; }
template<> inline       char* to_string(char* dst, size_t dst_size, const uint8_t& value)     { if (!dst || dst_size < 2) return nullptr; dst[0] = (char)value; dst[1] = '\0'; return dst; }
#define oDEFINE_TO_STRING_BUILTIN(_T, _Tfmt) template<> inline char* to_string(char* dst, size_t dst_size, const _T& value) { return -1 != snprintf(dst, dst_size, _Tfmt) ? dst : nullptr; }
oDEFINE_TO_STRING_BUILTIN(int16_t, "%hd" ) oDEFINE_TO_STRING_BUILTIN(uint16_t,      "%hu" )
oDEFINE_TO_STRING_BUILTIN(int32_t, "%d"  ) oDEFINE_TO_STRING_BUILTIN(uint32_t,      "%u"  )
oDEFINE_TO_STRING_BUILTIN(long,    "%d"  ) oDEFINE_TO_STRING_BUILTIN(unsigned long, "%u"  )
oDEFINE_TO_STRING_BUILTIN(int64_t, "%lld") oDEFINE_TO_STRING_BUILTIN(uint64_t,      "%llu")
oDEFINE_TO_STRING_BUILTIN(float,   "%f"  ) oDEFINE_TO_STRING_BUILTIN(double,        "%lf" )

#ifndef _MSC_VER
	#error sscanf_s is VS-specific
	#error _stricmp is VS-specific
#endif

template<> inline bool from_string(bool* out_value, const char* src)
{
	if (!_stricmp("true", src) || !_stricmp("t", src) || !_stricmp("yes", src) || !_stricmp("y", src))
		*out_value = true;
	else 
		*out_value = atoi(src) != 0;
	return true;
}

template<> inline bool from_string(char* out_value, const char* src) { *out_value = *src; return true; }
template<> inline bool from_string(uint8_t* out_value, const char* src) { *out_value = *(const uint8_t*)src; return true; }

#define oDEFINE_FROM_STRING_BUILTIN(_T, _Tfmt) template<> inline bool from_string(_T* out_value, const char* src) { return 1 == sscanf_s(src, _Tfmt, out_value); }
oDEFINE_FROM_STRING_BUILTIN(int16_t, "%hd" ) oDEFINE_FROM_STRING_BUILTIN(uint16_t, "%hu"    )
oDEFINE_FROM_STRING_BUILTIN(int32_t, "%d"  ) oDEFINE_FROM_STRING_BUILTIN(uint32_t, "%u"     )
oDEFINE_FROM_STRING_BUILTIN(long, "%d"     ) oDEFINE_FROM_STRING_BUILTIN(unsigned long, "%u")
oDEFINE_FROM_STRING_BUILTIN(int64_t, "%lld") oDEFINE_FROM_STRING_BUILTIN(uint64_t, "%llu"   )
oDEFINE_FROM_STRING_BUILTIN(float, "%f"    ) oDEFINE_FROM_STRING_BUILTIN(double, "%lf"      )

}
