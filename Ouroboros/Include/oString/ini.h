// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// ini document parser. Replaces delimiters inline with null terminators 
// and caches indices to the values

#pragma once
#include <oString/stringize.h>
#include <oString/text_document.h>
#include <cstdint>
#include <cstring>

namespace ouro {

class ini
{
	ini(const ini&);
	const ini& operator=(const ini&);

public:
	typedef uint16_t index_type;
	
	typedef struct section__ {}* section;
	typedef struct key__ {}* key;

	ini() : size_(0) {}
	ini(const char* uri, char* data, blob::deleter_fn deleter, size_t est_num_sections = 10, size_t est_num_keys = 100)
		: buffer_(uri, data, deleter)
	{
		size_ = sizeof(*this) + strlen(buffer_) + 1;
		entries_.reserve(est_num_sections + est_num_keys);
		index_buffer();
		size_ += entries_.capacity() * sizeof(index_type);
	}

	ini(const char* uri, const char* data, size_t est_num_sections = 10, size_t est_num_keys = 100)
	{
		size_t copy_size = strlen(data) + 1;
		char* copy = new char[copy_size];
		strlcpy(copy, data, copy_size);
		buffer_ = std::move(detail::text_buffer(uri, copy, copy_size, [](void* p) { delete p; }));
		size_ = sizeof(*this) + copy_size;
		entries_.reserve(est_num_sections + est_num_keys);
		index_buffer();
		size_ += entries_.capacity() * sizeof(index_type);
	}

	ini(ini&& that) { operator=(std::move(that)); }
	ini& operator=(ini&& that)
	{
		if (this != &that)
		{
			buffer_ = std::move(that.buffer_);
			entries_ = std::move(that.entries_);
			size_ = std::move(that.size_);
		}
		return *this;
	}

	inline operator bool() const { return (bool)buffer_; }
	inline const char* name() const { return buffer_.uri_; }
	inline size_t size() const { return size_;  }
	inline const char* section_name(section s) const { return buffer_.c_str() + entry(s).name; }
	inline section first_section() const { return entries_.size() > 1 ? section(1) : 0; }
	inline section next_section(section prior) const { return section(entry(prior).next); }
	inline key first_key(section s) const { return key(entry(s).value); }
	inline key next_key(key prior) const { return key(entry(prior).next); }
	inline const char* name(key k) const { return buffer_.c_str() + entry(k).name; }
	inline const char* value(key k) const { return buffer_.c_str() + entry(k).value; }

	inline section find_section(const char* name) const
	{
		section s = 0;
		if (name) for (s = first_section(); s && _stricmp(name, section_name(s)); s = next_section(s)) {}
		return s;
	}

	inline key find_key(section s, const char* key_name) const
	{
		key k = 0;
		if (s && key_name) for (k = first_key(s); k && _stricmp(key_name, name(k)); k = next_key(k)) {}
		return k;
	}

	inline key find_key(const char* section_name, const char* key_name) const { return find_key(find_section(section_name), key_name); }

	inline const char* find_value(section s, const char* key_name) const { return value(find_key(s, key_name)); }
	inline const char* find_value(const char* section_name, const char* key_name) const { return value(find_key(section_name, key_name)); }

	template<typename T> bool find_value(section s, const char* key_name, T* out_value) const
	{
		key k = find_key(s, key_name);
		if (k) return from_string(out_value, value(k));
		return false;
	}
	
	template<typename charT, size_t capacity>
	bool find_value(section s, const char* entry_name, fixed_string<charT, capacity>& out_value) const
	{
		key k = find_key(s, key_name);
		if (k) return out_value = value(k);
		return false;
	}

private:
  inline void index_buffer()
  {
	  char* c = buffer_;
	  index_type lastSectionIndex = 0;
	  entry_t s, k;
	  bool link = false;
	  entries_.push_back(entry_t()); // make 0 to be a null object

	  while (*c)
	  {
		  c += strspn(c, oWHITESPACE); // move past whitespace
		  // Check that we have stepped to the end of the file (this can happen if the last line of the file is blank (meaning lastine\r\n\r\n)
		  if (!*c)
			  break;
		  switch (*c)
		  {
			  case ';': // comment, move to end of line
				  c += strcspn(c, oNEWLINE), c++;
				  // If a comment is at the end of the file, we need to check to see if there is any file left to read
				  while (--c >= buffer_ && strchr(oWHITESPACE, *c)); // trim right whitespace
				  if (!*(++c)) break; // if there is no more, just exit
				  *c = 0;
				  c += 1 + strcspn(c, oNEWLINE); // move to end of line
				  // Support for \r\n  If we have just \n we need to advance 1 (done above), if we have both we need to advance a second time
				  if (*c == '\n')
					  c++; // advance again
				  break;
			  case '[': // start of section, record it
				  s.name = static_cast<index_type>(std::distance(buffer_.c_str(), c+1));
				  if (lastSectionIndex) entries_[lastSectionIndex].next = static_cast<index_type>(entries_.size());
				  lastSectionIndex = static_cast<index_type>(entries_.size());
				  entries_.push_back(s);
				  c += strcspn(c, "]"), *c++ = 0;
				  link = false;
				  break;
			  default:
				  if (entries_.size() <= 1) throw new text_document_error(text_document_errc::generic_parse_error);
				  k.next = 0;
				  k.name = static_cast<index_type>(std::distance(buffer_.c_str(), c));
				  c += strcspn(c, "=" oWHITESPACE); // move to end of key
				  bool atSep = *c == '=';
				  *c++ = 0;
				  if (!atSep) c += strcspn(c, "=") + 1; // if we moved to whitespace, continue beyond it
				  c += strspn(c, oWHITESPACE); // move past whitespace
				  k.value = static_cast<index_type>(std::distance(buffer_.c_str(), c));
				  c += strcspn(c, oNEWLINE ";"); // move to end of line or a comment
				  if (link) entries_.back().next = static_cast<index_type>(entries_.size());
				  link = true;
				  if (!entries_[lastSectionIndex].value) entries_[lastSectionIndex].value = static_cast<index_type>(entries_.size());
				  entries_.push_back(k);
				  while (--c >= buffer_ && strchr(oWHITESPACE, *c)); // trim right whitespace
				  if (!*(++c)) break; // if there is no more, just exit
				  *c = 0;
				  c += 1 + strcspn(c, oNEWLINE); // move to end of line
				  // Support for \r\n  If we have just \n we need to advance 1 (done above), if we have both we need to advance a second time
				  if (*c == '\n')
					  c++; // advance again
				  break;
		  }
	  }
	  *(char*)buffer_ = '\0'; // have all empty name/values point to 0 offset and now that offset will be the empty string
  }

  detail::text_buffer buffer_;
	struct entry_t
	{
		entry_t() : next(0), name(0), value(0) {}
		index_type next;
		index_type name;
		index_type value;
	};

  inline const entry_t& entry(key k) const { return entries_[(size_t)k]; }
	inline const entry_t& entry(section s) const { return entries_[(size_t)s]; }

	typedef detail::text_buffer::std_vector<entry_t> entries_t;
	entries_t entries_;
	size_t size_;
};

}
