// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// object that redirects file lifetime events to a functor

#pragma once
#include <oString/path.h>
#include <memory>
#include <cstdint>

namespace ouro { namespace filesystem {

enum class file_event
{
	unsupported,
	added,
	removed,
	modified,
	accessible,

	count,
};

class monitor
{
public:
	typedef void(*on_event_fn)(file_event event, const path_t& path, void* user);

	// A file can get an added or modified event before all work on the file is 
	// complete, so there is polling to check if the file has settled and is 
	// ready for access by another client system.
  struct info
  {
		info() : accessibility_poll_rate_ms(2000), accessibility_timeout_ms(5000) {}

    uint32_t accessibility_poll_rate_ms;
    uint32_t accessibility_timeout_ms;
  };

  static std::shared_ptr<monitor> make(const info& info, on_event_fn on_event, void* user);

  virtual info get_info() const = 0;
  
  // The specified path can be a wildcard. buffer_size is how much watch data
	// to buffer between sampling calls. This size is doubled to enable double-
	// buffering and better concurrency.
  virtual void watch(const path_t& path, size_t buffer_size, bool recursive) = 0; 
  
  // If a parent to the specified path is recursively watching, it will 
  // continue watching. This only removes entries that were previously
  // registered with watch().
  virtual void unwatch(const path_t& path) = 0;
};

}}
