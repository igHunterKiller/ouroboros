// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Describes an ordered list of regular expressions that refine a query 
// through a series of inclusion and exclusion patterns. 

// Example:
// Write a parsing tool that operates on C++ source this can be used to 
// handle command line options for including all symbols from source files 
// beginning with 's' but not std::vector symbols, except std::vector<SYMBOL>.
// ParseCpp -includefile "s.*" -excludesymbols "std\:\:vector.*" -includesymbols "std\:\:vector<SYMBOL>"

#pragma once
#include <regex>
#include <string>
#include <vector>

namespace ouro {

class filter_chain
{
public:
	enum class inclusion
	{
		exclude1,
		include1,
		exclude2,
		include2,
	};

	struct filter_t
	{
		const char* regex;
		inclusion type;
	};
	
	// ctors
	                     filter_chain()                                                                                      {}
	                     filter_chain(const filter_t* filters, size_t num_filters) : filters_(compile(filters, num_filters)) {}
	template<size_t num> filter_chain(const filter_t (&filters)[num])              : filters_(compile(filters, num))         {}
	                     filter_chain(filter_chain&& that)                         : filters_(std::move(that.filters_))      {}

	filter_chain& operator=(filter_chain&& that) { if (this != &that) { filters_ = std::move(that.filters_); } return *this; }

	// return true if either symbol1 or symbol2 passes all filters. A pass is a match 
	// to an include pattern or a mismatch to an exclude pattern.
	bool passes(const char* symbol1, const char* symbol2 = nullptr, bool passes_when_empty = true) const
	{
		if (filters_.empty()) return passes_when_empty;
		bool passes = filters_[0].first == inclusion::exclude1 || filters_[0].first == inclusion::exclude2;
		for (const auto& pair : filters_)
		{
			const char* s = symbol1;
			if (pair.first == inclusion::include2 || pair.first == inclusion::exclude2) s = symbol2;
			if (!s) passes = true;
			else if (std::regex_match(s, pair.second)) passes = (int)pair.first & 0x1; // incl enums are odd, excl are even.
		}
		return passes;
	}

private:
	typedef std::vector<std::pair<inclusion, std::regex> > filters_t;
	filters_t filters_;

	// returns an array of compiled regex's marked as include/exclude
	filters_t compile(const filter_t* filters, size_t num_filters)
	{
		filters_t f;
		f.reserve(num_filters);
		for (size_t i = 0; filters && filters->regex && i < num_filters; i++, filters++)
		{
			std::regex re;
			try { re = std::regex(filters->regex, std::regex_constants::icase); }
			catch (std::regex_error& e)
			{
				f.clear();
				std::string msg;
				msg.reserve(512);
				msg  = "filter_chain compile {";
				msg += filters->regex;
				msg += "} ";
				msg += e.what();
				throw std::invalid_argument(msg);
			}

			f.push_back(filters_t::value_type(filters->type, re));
		}

		return f;
	}
};

}
