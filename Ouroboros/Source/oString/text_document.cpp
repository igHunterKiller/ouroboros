// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oString/text_document.h>
#include <oArch/compiler.h>

namespace ouro
{
	namespace text_document_detail
	{
		class text_document_category_impl : public std::error_category
		{
		public:
			const char* name() const noexcept override { return "text_document"; }
			std::string message(int errcode) const override
			{
				switch (errcode)
				{
					case text_document_errc::generic_parse_error: return "generic_parse_error: an unspecified parsing error occurred";
					case text_document_errc::unclosed_scope: return "unclosed_scope: an open brace/tag was not matched by a closing brace/tag";
					case text_document_errc::unclosed_comment: return "unclosed_comment: a block comment was not closed with an ending tag";
					default: return "unrecognized text_document error";
				}
			}
		};
	}

	const std::error_category& text_document_category()
	{
		static text_document_detail::text_document_category_impl sSingleton;
		return sSingleton;
	}
}
