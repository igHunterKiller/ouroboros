// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Encapsulation of parsing a Universal Resource Identifier (uri_) that
// should be compliant with specification: http://tools.ietf.org/html/rfc3986

#pragma once
#include <oString/path.h>
#include <oCore/stringize.h>
#include <oString/string_codec.h>
#include <oString/uri_traits.h>
#include <oMemory/fnv1a.h>
#include <regex>

namespace ouro {
	namespace detail { std::regex& uri_regex(); }

template<typename charT, typename TraitsT>
class basic_uri_t
{
	typedef uint16_t index_type;
	static const index_type npos = ~0u & 0xffff;
	typedef std::pair<index_type, index_type> index_pair;

public:
	typedef TraitsT traits;
	typedef typename traits::path_traits_type path_traits_type;
	typedef typename traits::path_type path_type;
	typedef typename traits::char_type char_type;
	typedef typename traits::size_type size_type;
	static const size_type Capacity = traits::capacity;
	typedef fixed_string<char_type, 512> string_type;
	typedef uint64_t hash_type;
	typedef typename string_type::string_piece_type string_piece_type;
	typedef const char_type(&const_array_ref)[Capacity];

	static const char_type* remove_flag() { return (const char_type*)(-1); }

	basic_uri_t() { clear(); }
	basic_uri_t(const char_type* uri_ref) { operator=(uri_ref); }
	basic_uri_t(const char_type* uri_base, const char_type* uri_relative) { operator=(uri_relative); make_absolute(uri_base); }
	basic_uri_t(const basic_uri_t& that) { operator=(that); }
	basic_uri_t(const path_t& path) { operator=(path); }
	const basic_uri_t& operator=(const basic_uri_t& that)
	{
		if (this != &that)
		{
			uri_ = that.uri_;
			scheme_ = that.scheme_;
			authority_ = that.authority_;
			path_ = that.path_;
			query_ = that.query_;
			fragment_ = that.fragment_;
			hash_ = that.hash_;
		}
		return *this;
	}

	const basic_uri_t& operator=(const char_type* uri_ref)
	{
		if (uri_ref)
		{
			uri_ = uri_ref;
			parse();
		}
		else
			clear();
		return *this;
	}

	const basic_uri_t& operator=(const path_t& path)
	{
		string_type p(traits::file_scheme_prefix_str());
		const char_type *rootname = nullptr, *pPath = nullptr, *parentend = nullptr, *base = nullptr, *ext = nullptr;
		tsplit_path<char_type>(p, traits::path_traits_type::posix, &rootname, &pPath, &parentend, &base, &ext);
		if (!rootname)
			p += traits::sep_str();
		p += (path.c_str());
		return operator=(p);
	}

	// modifiers
	void clear()
	{
		uri_.clear();
		clear(scheme_);
		clear(authority_);
		clear(path_);
		clear(query_);
		clear(fragment_);
		end_ = 0;
		hash_ = 0;
	}

	void swap(basic_uri_t& that)
	{
		if (this != &that)
		{
			std::swap(scheme_, that.scheme_);
			std::swap(authority_, that.authority_);
			std::swap(path_, that.path_);
			std::swap(query_, that.query_);
			std::swap(fragment_, that.fragment_);
			std::swap(hash_, that.hash_);
		}
	}

	// If a parameter is not null, it will replace that part of the current 
	// basic_uri_t. If a parameter is remove_flag(), { that part will be removed.
	// (An empty string sets that part to a valid empty string part.)
	basic_uri_t& replace(
		const char_type* _scheme = nullptr
		, const char_type* _authority = nullptr
		, const char_type* _path = nullptr
		, const char_type* _query = nullptr
		, const char_type* _fragment = nullptr)
	{
		bool reparse = false;
		string_type s;
		if (_scheme == remove_flag()) { reparse = true; }
		else if (_scheme)
		{
			s += _scheme;
			to_lower(std::begin(s), std::end(s));
			s += traits::scheme_str();
		}
		else if (has_scheme())
		{
			s += convert(scheme_);
			s += traits::scheme_str();
		}

		if (traits::is_file_scheme(s))
		{
			s += traits::double_sep_str();

			if (_authority == remove_flag())
				;
			else if (_authority)
				s += _authority;
			else if (has_authority())
				s += convert(authority_);

			if (_path)
			{
				if (!path_traits_type::is_sep(_path[0]))
					s += traits::sep_str();
			}
			else if (has_path() && !path_traits_type::is_sep(*(uri_.c_str() + path_.first)))
				s += traits::sep_str();
		}
		else if (_authority == remove_flag()) { reparse = true; }
		else if (_authority)
		{
			s += traits::double_sep_str();
			s += _authority;
		}
		else if (has_authority())
		{
			s += traits::double_sep_str();
			s += convert(authority_);
		}

		if (_path == remove_flag()) { reparse = true; }
		else if (_path)
		{
			if (!s.empty() && _path && !path_traits_type::is_sep(*_path))
				s += traits::sep_str();

			path_type::string_type p;
			clean_path(p, _path);
			size_t len = s.length();
			percent_encode(s.c_str() + len, s.capacity() - len, p, " ");
		}
		else if (has_path())
			s.append(convert(path_));

		if (_query == remove_flag()) { reparse = true; }
		else if (_query)
		{
			s += traits::query_str();
			s += _query;
		}
		else if (has_query())
		{
			s += traits::query_str();
			s.append(convert(query_));
		}
		
		if (_fragment == remove_flag()) { reparse = true; }
		else if (_fragment)
		{
			s += traits::fragment_str();
			s += _fragment;
		}
		else if (has_fragment())
		{
			s += traits::fragment_str();
			s.append(convert(fragment_));
		}

		if (reparse || !s.empty())
			operator=(s);

		return *this;
	}

	basic_uri_t& replace_scheme(const char_type* _scheme = traits::empty_str()) { return replace(_scheme); }
	basic_uri_t& replace_authority(const char_type* _authority = traits::empty_str()) { return replace(nullptr, _authority); }
	basic_uri_t& replace_path(const char_type* _path = traits::empty_str()) { return replace(nullptr, nullptr, _path); }
	basic_uri_t& replace_query(const char_type* _query = traits::empty_str()) { return replace(nullptr, nullptr, nullptr, _query); }
	basic_uri_t& replace_fragment(const char_type* _fragment = traits::empty_str()) { return replace(nullptr, nullptr, nullptr, nullptr, _fragment); }

	basic_uri_t& replace_extension(const char_type* _NewExtension = traits::empty_str())
	{
		return replace_path(path().replace_extension(_NewExtension));
	}

	basic_uri_t& replace_extension_with_suffix(const char_type* _NewSuffix)
	{
		return replace_path(path().replace_extension_with_suffix(_NewSuffix));
	}

	basic_uri_t& replace_filename(const char_type* _NewFilename = traits::empty_str())
	{
		return replace_path(path().replace_filename(_NewFilename));
	}

	// Returns a basic_uri_t reference relative to the specified uri_base. If this basic_uri_t 
	// does not begin with the base, the returned basic_uri_t is empty.
	basic_uri_t& make_absolute(const basic_uri_t& base_uri)
	{
		bool usebasescheme = false;
		bool usebaseauthority = false;
		bool usemergedpath = false;
		bool usebasequery = false;
		path_type mergedpath;

		if (!has_scheme())
		{
			if (!has_authority())
			{
				if (empty(path_))
				{
					mergedpath = base_uri.path();
					usemergedpath = true;
					usebasequery = !has_query();
				}
				
				else
				{
					if (!path_traits_type::is_sep(*(uri_.c_str() + path_.first)))
					{
						mergedpath = base_uri.path();
						mergedpath.remove_filename();
						mergedpath.append(convert(path_));
						usemergedpath = true;
					}
				}
				usebaseauthority = true;
			}
			usebasescheme = true;
		}

		return replace(
			usebasescheme ? (base_uri.has_scheme() ? base_uri.scheme().c_str() : remove_flag()) : nullptr
			, usebaseauthority ? (base_uri.has_authority() ? base_uri.authority().c_str() : remove_flag()) : nullptr
			, usemergedpath ? mergedpath.c_str() : nullptr
			, usebasequery ? (base_uri.has_query() ? base_uri.query().c_str() : remove_flag()) : nullptr);
	}

	basic_uri_t& make_relative(const basic_uri_t& uri_base)
	{
		auto s = scheme(), bs = uri_base.scheme();
		auto a = authority(), ba = uri_base.authority();

		// Must have scheme and authority_ the same to be made relative.
		if (traits::compare(s, bs) || traits::compare(a, ba))
			return *this;

		string_type relpath;
		auto p = path(), bp = uri_base.path();
		if (!relativize_path(relpath, bp, p))
			return *this;

		return replace(remove_flag(), remove_flag(), relpath);
	}

	// decomposition
	const_array_ref c_str() const { return (const_array_ref)uri_; }
	operator const char_type*() const { return uri_; }
	operator string_type() const { return uri_; }

	string_piece_type scheme_piece() const { return convert(scheme_); }
	string_piece_type authority_piece() const { return convert(authority_); }
	string_piece_type path_piece() const { return convert(path_); }
	string_piece_type query_piece() const { return convert(query_); }
	string_piece_type fragment_piece() const { return convert(fragment_); }

	string_type scheme() const { return string_type(convert(scheme_)); }
	string_type authority() const { return string_type(convert(authority_)); }
	string_type path_string() const { string_type s(convert(path_)); percent_decode(s, s.capacity(), s); return s; }
	string_type path_encoded() const { return string_type(convert(path_)); }
	path_type path() const { return path_type(path_string()); }
	string_type query() const { return string_type(convert(query_)); }
	string_type fragment() const { return string_type(convert(fragment_)); }

	// culls the fragment
	basic_uri_t document() const { return has_fragment() ? basic_uri_t(uri_.c_str(), uri_.c_str() + fragment_.first - 1) : *this; }

	// todo: implement user_info, host, port

	// query
	/* constexpr */ size_type capacity() const { return Capacity; }
	bool empty() const { return uri_.empty(); }
	hash_type hash() const { return hash_; }
	bool valid() const { return has_scheme() || has_authority() || has_path() || has_query() || has_fragment(); }
	bool relative() const { return !absolute(); }
	bool absolute() const { return has_scheme() && empty(fragment_); }
	operator bool() const { return valid(); }
	operator size_type() const { return static_cast<size_type>(hash()); }
	bool has_scheme() const { return !empty(scheme_); }
	bool has_authority() const { return !empty(authority_); }
	bool has_path() const { return !empty(path_); }
	bool has_query() const { return !empty(query_); }
	bool has_fragment() const { return !empty(fragment_); }

	// Compares the scheme, authority, path and query, (ignores fragment) NOTE: 
	// a.is_same_docment(b) != b.is_same_document(a) The "larger" more explicit 
	// document should be the parameter and this should hold the smaller/relative
	// part. http://tools.ietf.org/html/rfc3986#section-4.4
	bool is_same_document(const basic_uri_t& that) const
	{
		if (empty() || that.empty() || uri_[0] == *traits::fragment_str() || that.uri_[0] == *traits::fragment_str())
			return true;
		basic_uri_t tmp(*this, that);
		return (!traits::compare(tmp.scheme(), that.scheme())
			&& !traits::compare(tmp.authority(), that.authority())
			&& !traits::compare(tmp.path(), that.path())
			&& !traits::compare(tmp.query(), that.query()));
	}

	int compare(const basic_uri_t& that) const
	{
		if (hash_ == that.hash_)
		{
			#ifdef _DEBUG
				if (traits::compare(uri_, that.uri_))
				{
					char msg[oMAX_URI*2 + 64];
					snprintf(msg, "has collision between \"%s\" and \"%s\"", uri_.c_str(), that.uri_.c_str());
					throw std::logic_error(msg);
				}
			#endif
			return 0;
		}
		return traits::compare(uri_, that.uri_);
	}

	bool operator==(const basic_uri_t& that) const { return compare(that) == 0; }
	bool operator!=(const basic_uri_t& that) const { return !(*this == that); }
	bool operator<(const basic_uri_t& that) const { return compare(that) < 0; }

private:

	fixed_string<char_type, Capacity> uri_;
	index_pair scheme_, authority_, path_, query_, fragment_;
	index_type end_;
	hash_type hash_;

	template<typename BidirIt>
	index_pair convert(const std::sub_match<BidirIt>& _Submatch) const
	{
		return index_pair(
			static_cast<index_type>(_Submatch.first - uri_.c_str())
			, static_cast<index_type>(_Submatch.second - uri_.c_str()));
	}

	static bool empty(const index_pair& pair) { return pair.first == pair.second; }

	string_piece_type convert(const index_pair& pair) const
	{
		return string_piece_type(uri_.c_str() + pair.first, uri_.c_str() + pair.second);
	}

	void clear(index_pair& pair) { pair.first = pair.second = 0; }

	void parse()
	{
		const std::regex& reURI = detail::uri_regex();
		std::match_results<const char_type*> matches;
		std::regex_search(uri_.c_str(), matches, reURI);
		if (matches.empty())
			oThrow(std::errc::invalid_argument, "invalid basic_uri_t");

		// apply good practices from http://www.textuality.com/tag/uri-comp-2.html
		bool hadprefix = matches[2].matched || matches[4].matched;
		bool hasprefix = hadprefix;
		// Clean path of non-leading . and .. and scrub separators
		if (matches[5].length() && !path_traits_type::is_dot(uri_[0]))
		{
			// const-cast of the template type so path cleaning can be done in-place
			std::match_results<char_type*>::value_type& path_match = 
				*(std::match_results<char_type*>::value_type*)&matches[5];

			char_type c = *path_match.second;
			*path_match.second = 0;
			ouro::path_string p;
			clean_path(path_match.first, path_match.length() + 1, path_match.first); // length() + 1 is for nul
			size_t newLen = string_type::traits::length(path_match.first);
			*path_match.second = c;
			if (newLen != static_cast<size_t>(path_match.length()))
			{
				memmove(path_match.first + newLen, path_match.second, std::distance(matches[5].second, matches[0].second));
				std::regex_search(uri_.c_str(), matches, reURI);
				hasprefix = matches[2].matched || matches[4].matched;
			}
		}

		// http://tools.ietf.org/html/rfc3986#appendix-B
		// if there was a prefix change due to path cleaning, it means there was
		// never a prefix
		if (hadprefix != hasprefix)
		{
			scheme_ = index_pair();
			authority_ = index_pair();
		}
		else
		{
			scheme_ = convert(matches[2]);
			authority_ = convert(matches[4]);
		}

		path_ = convert(matches[5]);
		query_ = convert(matches[7]);
		fragment_ = convert(matches[9]);
		end_ = static_cast<index_type>(matches[0].second - matches[0].first);

		// const-cast of the template type so to-lower can be done in-place
		// for now do the whole uri_, but at least the scheme and percent encodings
		// should be forced to lower-case.
		std::match_results<char_type*>::value_type& to_lower_range = 
			*(std::match_results<char_type*>::value_type*)&matches[2];

		to_lower(to_lower_range.first, to_lower_range.second);

		// If the whole path isn't made lower-case for case-insensitive purposes, 
		// the percent values at least should be made lower-case (i.e. this should 
		// be true: &7A == &7a)
		percent_to_lower(uri_, uri_.capacity(), uri_);

		hash_ = fnv1a<hash_type>(uri_);
	}
};

typedef basic_uri_t<char, uri_traits<char, unsigned long long, default_posix_path_traits<char>>> uri_t;
typedef basic_uri_t<wchar_t, uri_traits<wchar_t, unsigned long long, default_posix_path_traits<wchar_t>>> wuri_t;

template<typename charT, typename TraitsT>
char* to_string(char* dst, size_t dst_size, const basic_uri_t<charT, TraitsT>& value)
{
	try { ((const basic_uri_t<charT, TraitsT>::string_type&)value).copy_to(dst, dst_size); }
	catch (std::exception&) { return nullptr; }
	return dst;
}

template<typename charT, typename TraitsT>
bool from_string(basic_uri_t<charT, TraitsT>* out_value, const char* src)
{
	try { *out_value = src; }
	catch (std::exception&) { return false; }
	return true;
}

}

namespace std {

template<typename charT, typename TraitsT>
struct hash<ouro::basic_uri_t<charT, TraitsT>> { std::size_t operator()(const ouro::basic_uri_t<charT, TraitsT>& _uri) const { return _uri.hash(); } };

}
