// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <string.h>
#include <malloc.h>

namespace ouro {

char* insert(char* src, size_t src_size, char* insertion_point, size_t replacement_length, const char* insertion)
{
	size_t insertionLength = strlen(insertion);
	size_t afterInsertionLength = strlen(insertion_point) - replacement_length;
	size_t newLen = static_cast<size_t>(insertion_point - src) + afterInsertionLength;
	if (newLen >= src_size)
		return nullptr;
	// to avoid the overwrite of a direct memcpy, copy the remainder
	// of the string out of the way and then copy it back in.
	char* tmp = (char*)alloca(afterInsertionLength);
	memcpy(tmp, insertion_point + replacement_length, afterInsertionLength);
	memcpy(insertion_point, insertion, insertionLength);
	char* p = insertion_point + insertionLength;
	memcpy(p, tmp, afterInsertionLength);
	p[afterInsertionLength] = '\0';
	return p;
}

}
