// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Object that redirects file lifetime events to a functor.

#pragma once
#include <oString/path.h>
#include <memory>

namespace ouro { namespace filesystem {

/* enum class */ namespace file_event
{	enum value {

	unsupported,
	added,
	removed,
	modified,
	accessible,

};}

class monitor
{
public:
	// A file can get an added or modified event before all work on the file is 
	// complete, so there is polling to check if the file has settled and is 
	// ready for access by another client system.
  struct info
  {
		info()
			: accessibility_poll_rate_ms(2000)
			, accessibility_timeout_ms(5000)
		{}

    unsigned int accessibility_poll_rate_ms;
    unsigned int accessibility_timeout_ms;
  };

  static std::shared_ptr<monitor> make(const info& _Info, const std::function<void(file_event::value _Event, const path_t& _Path)>& _OnEvent);

  virtual info get_info() const = 0;
  
  // The specified path can be a wildcard. _BufferSize is how much watch data
	// to buffer between sampling calls. This size is doubled to enable double-
	// buffering and better concurrency.
  virtual void watch(const path_t& _Path, size_t _BufferSize, bool _Recursive) = 0; 
  
  // If a parent to the specified path is recursively watching, it will 
  // continue watching. This only removes entries that were previously
  // registered with watch().
  virtual void unwatch(const path_t& _Path) = 0;
};

}}
