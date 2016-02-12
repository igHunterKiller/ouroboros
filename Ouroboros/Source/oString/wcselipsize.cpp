// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

namespace ouro {

wchar_t* wcsellipsize(wchar_t* dst, size_t _NumChars)
{
	switch (_NumChars - 4)
	{ // Duff's device (no breaks)
		default: dst[_NumChars-4] = L'.';
		case 3: dst[_NumChars-3] = L'.';
		case 2: dst[_NumChars-2] = L'.';
		case 1: dst[_NumChars-1] = 0;
		case 0: break;
	}

	return dst;
}

}
