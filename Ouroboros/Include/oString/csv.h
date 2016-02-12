// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// csv document parser. Replaces delimiters inline with null terminators 
// and caches indices to the values

#pragma once
#include <oString/text_document.h>
#include <cstdint>
#include <cstring>

namespace ouro {

class csv
{
public:
	typedef uint32_t index_type;
	
	csv(const char* uri, char* data, blob::deleter_fn deleter, size_t est_num_rows = 100, size_t est_num_cols = 20)
		: buffer_(uri, data, deleter)
		, num_cols_(0)
	{
		size_ = sizeof(*this) + strlen(buffer_) + 1;
		cells_.reserve(est_num_rows);
		index_buffer(est_num_cols);
		for (const auto& cell : cells_)
			size_ += cell.capacity() * sizeof(index_type);
	}

	csv(const char* uri, const char* data, size_t est_num_rows = 100, size_t est_num_cols = 20)
		: num_cols_(0)
	{
		size_t copy_size = strlen(data) + 1;
		char* copy = new char[copy_size];
		strlcpy(copy, data, copy_size);
		buffer_ = std::move(detail::text_buffer(uri, copy, copy_size, [](void* p) { delete p; }));
		size_ = sizeof(*this) + strlen(buffer_) + 1;
		cells_.reserve(est_num_rows);
		index_buffer(est_num_cols);
		for (const auto& cell : cells_)
			size_ += cell.capacity() * sizeof(index_type);
	}

	inline const char* name() const { return buffer_.uri_; }
	inline size_t size() const { return size_; }
	inline size_t rows() const { return cells_.size(); }
	inline size_t cols() const { return num_cols_; }
	inline const char* cell(size_t col, size_t row) const { return (row < cells_.size() && col < cells_[row].size()) ? buffer_.c_str() + cells_[row][col] : ""; }

private:
	inline void index_buffer(size_t est_num_cols)
  {
	  cells_.resize(1);
	  bool inquotes = false;
	  char* c = buffer_;
	  char* start = c;
	  while (1)
	  {
		  if (*c == '\"')
		  {
			  if (!inquotes) inquotes = true;
			  else if (*(c+1) != '\"') inquotes = false;
			  c++;
		  }

		  if (strchr(oNEWLINE, *c) || *c == '\0')
		  {
			  const bool done = *c == '\0';
			  if (inquotes) throw new text_document_error(text_document_errc::generic_parse_error);
			  *c++ = '\0';
			  cells_.back().push_back(static_cast<index_type>(std::distance(buffer_.c_str(), start)));
			  start = c;
			  if (done) break;
			  else
			  {
				  c += strspn(c, oNEWLINE);
				  cells_.resize(cells_.size() + 1);
				  cells_.back().reserve(cells_[cells_.size()-2].size());
			  }
		  }

		  else
		  {
			  if (!inquotes)
			  {
				  if (*c == ',')
				  {
					  *c++ = '\0';
					  cells_.back().push_back(static_cast<index_type>(std::distance(buffer_.c_str(), start)));
					  start = c;
				  }
				  else
					  c++;
			  }
			  else
				  c++;
		  }
	  }

	  for (const auto& cell : cells_)
		  num_cols_ = std::max(num_cols_, cell.size());
  }

  detail::text_buffer buffer_;
	detail::text_buffer::std_vector<detail::text_buffer::std_vector<index_type>> cells_;
	size_t size_;
	size_t num_cols_;
};

}
