// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// base class for text document parsers that operate on a string (no file I/O)

#pragma once
#include <oCore/blob.h>
#include <oString/fixed_string.h>
#include <system_error>
#include <vector>

namespace ouro {

enum class text_document_errc { generic_parse_error, unclosed_scope, unclosed_comment };

const std::error_category& text_document_category();

/*constexpr*/ inline std::error_code make_error_code(text_document_errc err_code) { return std::error_code(static_cast<int>(err_code), text_document_category()); }
/*constexpr*/ inline std::error_condition make_error_condition(text_document_errc err_code) { return std::error_condition(static_cast<int>(err_code), text_document_category()); }

class text_document_error : public std::logic_error
{
public:
	text_document_error(text_document_errc err_code) 
		: logic_error(text_document_category().message(static_cast<int>(err_code))) {}
};

namespace detail {

class text_buffer : public blob
{
public:

	// todo: make this have an allocator everything comes from
	template<typename T>
	class std_vector : public std::vector<T> {};

	text_buffer() {}

	text_buffer(const char* uri, char* data, const blob::deleter_fn& deleter)
		: blob(data, data ? strlen(data) : 0, deleter)
		, uri_(uri)
	{}

	text_buffer(const char* uri, char* data, size_t data_size, const blob::deleter_fn& deleter)
		: blob(data, data_size, deleter)
		, uri_(uri)
	{}

	text_buffer(text_buffer&& that)
		: blob(std::move(that))
		, uri_(std::move(that.uri_))
	{}

	text_buffer& operator=(text_buffer&& that)
	{
		blob::operator=(std::move(that));
		if (this != &that)
			uri_ = std::move(that.uri_);
		return *this;
	}

	operator bool() const { return blob::operator bool(); }

	operator char*() { return (char*)ptr(); }
	operator const char*() const { return (const char*)ptr(); }

	const char* c_str() const { return (const char*)ptr(); }
	char* c_str() { return (char*)ptr(); }

	const char* uri() const { return uri_.c_str(); }

	uri_string uri_;
private:
	text_buffer(const text_buffer&);
	const text_buffer& operator=(const text_buffer&);
};

}}
