// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.
// Uses msgbox to report errors in a form that can be passed to reporting
#pragma once

#include <oCore/assert.h>

namespace ouro {

assert_action prompt_msgbox(const assert_context& assertion, const char* message);

}
