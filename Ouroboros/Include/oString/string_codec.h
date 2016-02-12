// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// string encode/decode functions

#pragma once
#include <oArch/compiler.h>

#define oRESERVED_URI_CHARS "!*'();:@&=+$,?#[]/"
#define oRESERVED_URI_CHARS_WITH_SPACE oRESERVED_URI_CHARS " "

namespace ouro {

// encode a string with URI-compliant percent encoding. 
// dst and src must not overlap.
char* percent_encode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src, const char* oRESTRICT reserved_chars = oRESERVED_URI_CHARS);
template<size_t size> char* percent_encode(char (&dst)[size], const char* src) { return percent_encode(dst, size, src, reserved_chars); }

// decode a string encoded with URI-compliant percent encoding.
// dst and src can be the same pointer.
char* percent_decode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src);
template<size_t size> char* percent_decode(char (&dst)[size], const char* src) { return percent_decode(dst, size, src); }

// ensures all percent encodings use lower-case letter values. For exmaple this
// will convert %7A to %7a. // dst and src can be the same 
// pointer. (NOTE: percent_encode always returns lower-case values, so this is
// only necessary on percent-encoded strings that were not encoded with 
// percent_encode.)
char* percent_to_lower(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src);
template<size_t size> char* percent_to_lower(char (&dst)[size], const char* src) { return percent_to_lower(dst, size, src); }

// encode a string with XML-compliant ampersand encoding.
char* ampersand_encode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src);
template<size_t size> char* ampersand_encode(char (&dst)[size], const char* src) { return ampersand_encode(dst, size, src); }

// decode a string encoded with XML-compliant ampersand encoding.
char* ampersand_decode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src);
template<size_t size> char* ampersand_decode(char (&dst)[size], const char* src) { return ampersand_decode(dst, size, src); }

// encode a string with JSON-compliant escape encoding.
char* json_escape_encode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src);
template<size_t size> char* json_escape_encode(char (&dst)[size], const char* src) { return json_escape_encode(dst, size, src); }

// decode a string encoded with JSON-compliant escape encoding.
char* json_escape_decode(char* oRESTRICT dst, size_t dst_size, const char* oRESTRICT src);
template<size_t size> char* json_escape_decode(char (&dst)[size], const char* src) { return json_escape_decode(dst, size, src); }

}
