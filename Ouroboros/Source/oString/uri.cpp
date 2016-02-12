// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
#include <oString/uri.h>

// http://tools.ietf.org/html/rfc3986#appendix-B
static std::regex reURI("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?", std::regex_constants::optimize); // ok static (duplication in dynamic libswon't affect correctness)

namespace ouro { namespace detail {

		std::regex& uri_regex()
		{
			// don't use singleton because this allocates memory and can return a false
			// positive memory leak.
			return reURI;
		}

}}
