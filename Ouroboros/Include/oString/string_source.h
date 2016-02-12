// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// string manipulation functions for lightweight parsing cpp, shader 
// and similar language source code.

#pragma once
#include <oArch/compiler.h>

namespace ouro {

// Like this very C++ comment! This function replaces the comment with the 
// specified char. comment_prefix for C++ would be "//" but could also be ";" or 
// "'" or "--" for other languages.
char* zero_line_comments(char* oRESTRICT str, const char* oRESTRICT comment_prefix, char replacement = ' ');

// Zeros-out all text sections delimited by the open and close braces, useful for 
// getting rid of block comments or #if 0/#endif blocks. This preserves newlines so 
// accurate line numbers can be reported in case of subsequent compile errors.
char* zero_block_comments(char* str, const char* open_brace, const char* close_brace, char replacement = ' ');

// This function uses the specified macros to go through and evaluate C-style
// #if* statements (#if, #ifdef, #elif, #else, #endif) to zero out undefined
// code. The final macro should be { nullptr, nullptr } as a nul terminator.
struct macro
{
	const char* symbol;
	const char* value;
};

char* zero_ifdefs(char* oRESTRICT str, const macro* oRESTRICT macros, char replacement);

// Combine the above 3 functions for the typical use case
inline char* zero_non_code(char* oRESTRICT str, const macro* oRESTRICT macros, char replacement = ' ')
{
	if (zero_block_comments(str, "/*", "*/", replacement)
		&& zero_ifdefs(str, macros, replacement)
		&& zero_line_comments(str, "//", replacement))
			return str;
	return nullptr;
}

// Convert a buffer into a C++ array. This is useful when you want to embed data 
// in code itself. This fills the destination string with a declaration of the 
// form:
// const <specifiedType> <specifiedName>[] = { <buffer data> };
// This also defines a function of the form:
// void get_<buffer_name>(const char** ppBufferName, const void** ppBuffer, size_t* pSize)
// that can be externed and used to access the buffer. Any extension '.' in the
// specified bufferName will be replaced with '_', so get_MyFile_txt(...)

size_t codify_data(char* oRESTRICT dst
									 , size_t dst_size
									 , const char* oRESTRICT buffer_name
									 , const void* oRESTRICT buf
									 , size_t buf_size
									 , size_t word_size);

}
