// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// The macros below are an inner-loop optimization with 5+% performance 
// implications, so please bear with the oddity. Basically this set of API is a 
// specialization of strspn and strcspn for text-file/string parsing to move 
// between whitespace-delimited words where a word is a series of characters 
// that are not whitespace. This is for parsing large data files like OBJ 3D 
// model files or for parsing C++/HLSL in small compilation efforts, or for just
// having fast next-word parsing for any text data. The LUT nature of the 
// implementations has a big impact, but factoring this code out undid a lot of
// the impact, so the solution was to factor definition of all this into a macro
// so code locality advantages can be retained. Call oDEFINE_WHITESPACE_PARSING() 
// in a .cpp file and use the API documented below for pretty-darn-fast parsing 
// capabilities.

#pragma once
#include <oArch/compiler.h> // oALIGNAS

// Returns true if c is any kind of whitespace
/// inline bool is_whitespace(int c)

// Returns true if c is whitespace that is not a newline
/// inline bool is_line_whitespace(int c)

// Returns true if c is a newline
/// inline bool is_newline(int c)

// Updates the specified pointer to a string until it points at a non-whitespace 
// character or a newline. To reiterate, this does not skip newline whitespace.
/// inline void move_past_line_whitespace(const char** _ppString)

// Updates the specified pointer to a string until it points at a whitespace 
// character, either line or newline whitespace.
/// inline void move_to_whitespace(const char** _ppString);

// Updates the specified pointer to consume the current non-whitespace and then 
// skip any other non-newline whitespace, thus pointing at the next word, i.e.
// next non-whitespace block of characters that is not the current block of 
// characters.
/// inline void move_next_word(const char** _ppString);

// Updates the specified pointer to a string until it points at a newline 
// character.
/// inline void move_to_line_end(const char** _ppString);

// Updates the specified pointer to a string until it points at the first non-
// newline character.
/// inline void move_past_newline(const char** _ppString);

#define oZ16 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#define oZ16_12 oZ16, oZ16, oZ16, oZ16, oZ16, oZ16, oZ16, oZ16, oZ16, oZ16, oZ16, oZ16
#define oDEFINE_WHITESPACE_FUNCTION(_Name, _Criteria) inline void _Name(const char** out_str) { while (**out_str && _Criteria(**out_str)) ++*out_str; }
#define oDEFINE_WHITESPACE_PARSING() \
	namespace { \
		oALIGNAS(16) static const unsigned char is_whitespace__[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, oZ16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, oZ16, oZ16_12 }; \
		oALIGNAS(16) static const unsigned char is_line_whitespace__[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 0, oZ16, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, oZ16, oZ16_12 }; \
		oALIGNAS(16) static const unsigned char is_newline__[256] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, oZ16, oZ16, oZ16, oZ16_12 }; \
	} namespace ouro { \
		inline bool is_whitespace(int c) { return !!is_whitespace__[c]; } \
		inline bool is_line_whitespace(int c) { return !!is_line_whitespace__[c]; } \
		inline bool is_newline(int c) { return !!is_newline__[c]; } \
		oDEFINE_WHITESPACE_FUNCTION(move_past_line_whitespace, is_line_whitespace) \
		oDEFINE_WHITESPACE_FUNCTION(move_to_whitespace, !is_whitespace) \
		oDEFINE_WHITESPACE_FUNCTION(move_to_line_end, !is_newline) \
		oDEFINE_WHITESPACE_FUNCTION(move_past_newline, is_newline) \
		inline void move_next_word(const char** out_str) { move_to_whitespace(out_str); move_past_line_whitespace(out_str); } \
	}
