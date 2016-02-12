// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

namespace ouro {

bool is_ascii(const void* buf, size_t buf_size)
{
	// http://code.activestate.com/recipes/173220-test-if-a-file-or-string-is-text-or-binary/
	// "The difference between text and binary is ill-defined, so this duplicates 
	// "the definition used by Perl's -T flag, which is: <br/> The first block 
	// "or so of the file is examined for odd characters such as strange control
	// "codes or characters with the high bit set. If too many strange characters 
	// (>30%) are found, it's a -B file, otherwise it's a -T file. Also, any file 
	// containing null in the first block is considered a binary file."
	static const float kThreshold = 0.10f; // 0.30f; // 30% seems too high to me.

	// Count non-text characters
	size_t nonTextCount = 0;
	const char* b = static_cast<const char*>(buf);
	for (size_t i = 0; i < buf_size; i++)
		if (b[i] == 0 || (b[i] & 0x80))
			nonTextCount++;

	float percentNonAscii = nonTextCount / static_cast<float>(buf_size);
	return percentNonAscii < kThreshold;
}

}
