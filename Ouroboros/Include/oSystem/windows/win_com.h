// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#pragma once

// COM (Window's component object model)

namespace ouro { namespace windows { namespace com {

// Needs to be call per-thread that uses COM
void ensure_initialized();
		
}}}
