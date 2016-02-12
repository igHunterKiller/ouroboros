// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// printf-style interface for displaying platform message boxes

#pragma once
#include <oGUI/oGUI.h>
#include <stdarg.h>

namespace ouro {

  enum class msg_type
  {
    info,
    warn,
    error,
    debug,
    yesno,
    notify,
    notify_info,
    notify_warn,
    notify_error,

		count,
  };
  
  enum class msg_result
  {
    no,
    yes,
    abort,
    debug,
    ignore,
    ignore_always,

		count,
  };

	msg_result msgboxv(msg_type _Type, ouro::window_handle _hParent, const char* _Title, const char* _Format, va_list _Args);
  inline msg_result msgbox(msg_type _Type, ouro::window_handle _hParent, const char* _Title, const char* _Format, ...) { va_list args; va_start(args, _Format); msg_result r = msgboxv(_Type, _hParent, _Title, _Format, args); va_end(args); return r; }

}
