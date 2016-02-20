// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Encapsulation of parsing a string representing a file on a local hard drive.

#pragma once
#include <oString/fixed_string.h>
#include <oString/path_traits.h>
#include <oString/stringize.h>
#include <functional>

namespace ouro {

template<typename charT, typename traitsT = default_posix_path_traits<charT>>
class basic_path_t
{
public:
	static const size_t Capacity = oMAX_PATH;
	typedef typename traitsT traits;
	typedef typename traits::char_type char_type;
	typedef size_t hash_type;
	typedef size_t size_type;
	typedef path_string string_type;
	typedef std::pair<const char_type*, const char_type*> string_piece_type;

	typedef const char_type(&const_array_ref)[Capacity];

	basic_path_t() { clear(); }
	basic_path_t(const char_type* start, const char_type* end) : p(start, end) { parse(); }
	basic_path_t(const char_type* that) { operator=(that); }
	basic_path_t(const basic_path_t& that) { operator=(that); }
	basic_path_t& operator=(const basic_path_t& that) { return operator=(that.c_str()); }
	basic_path_t& operator=(const char_type* that)
	{
		if (that)
		{
			p = that;
			parse();
		}
		else
			clear();
		return *this;
	}

	// modifiers

	void clear()
	{
		p.clear();
		length_ = 0;
		root_dir_offset_ = parent_end_offset_ = basename_offset_ = extension_offset_ = npos;
		has_root_name_ = ends_with_sep_ = false;
		hash_ = 0;
	}

	basic_path_t& assign(const char_type* start, const char_type* end) { p.assign(start, end); parse(); return *this; }

	basic_path_t& append(const char_type* path_element, bool ensure_separator = true)
	{
		if (ensure_separator && !traits::is_sep(*path_element) && !ends_with_sep_ && p.length())
			p.append(traits::generic_sep_str());
		p.append(path_element);
		parse();
		return *this;
	}

	basic_path_t& append(const char_type* start, const char_type* end, bool ensure_separator = true)
	{
		if (ensure_separator && !traits::is_sep(*start) && !ends_with_sep_ && p.length())
			p.append(traits::generic_sep_str());
		p.append(start, end);
		parse();
		return *this;
	}

	basic_path_t& append(const string_piece_type& string_piece, bool ensure_separator = true) { return append(string_piece.first, string_piece.second, ensure_separator); }

	void swap(basic_path_t& that)
	{
		if (this != &that)
		{
			std::swap(p, that.p);
			std::swap(root_dir_offset_, that.root_dir_offset_);
			std::swap(parent_end_offset_, that.parent_end_offset_);
			std::swap(basename_offset_, that.basename_offset_);
			std::swap(extension_offset_, that.extension_offset_);
			std::swap(has_root_name_, that.has_root_name_);
			std::swap(ends_with_sep_, that.ends_with_sep_);
		}
	}

	basic_path_t operator/(const basic_path_t& that) const
	{
		basic_path_t path(*this);
		return path /= that;
	}

	basic_path_t operator/(const char_type* that) const
	{
		basic_path_t path(*this);
		return path.append(that);
	}

	basic_path_t& operator/=(const char_type* that)
	{
		append(that);
		return *this;
	}

	basic_path_t& replace_extension(const char_type* new_ext = traits::empty_str())
	{
		if (has_extension()) p[extension_offset_] = 0;
		if (new_ext && !traits::is_dot(new_ext[0])) p.append(traits::dot_str());
		p.append(new_ext);
		parse();
		return *this;
	}

	basic_path_t& replace_extension(const string_type& new_ext)
	{
		return replace_extension(new_ext.c_str());
	}

	basic_path_t& replace_extension_with_suffix(const char_type* new_ext = traits::empty_str())
	{
		if (has_extension()) p[extension_offset_] = 0;
		p.append(new_ext);
		parse();
		return *this;
	}

	basic_path_t& replace_extension_with_suffix(const string_type& new_ext)
	{
		return replace_extension_with_suffix(new_ext.c_str());
	}

	basic_path_t& replace_filename(const char_type* new_filename = traits::empty_str())
	{
		if (basename_offset_ != npos)
			p[basename_offset_] = 0;
		else if (extension_offset_ != npos)
			p[extension_offset_] = 0;
		p.append(new_filename);
		parse();
		return *this;
	}

	basic_path_t& replace_filename(const string_type& new_filename)
	{
		return replace_filename(new_filename.c_str());
	}

	basic_path_t& replace_filename(const basic_path_t& new_filename)
	{
		return replace_filename(new_filename.c_str());
	}

	basic_path_t& remove_filename()
	{
		if (basename_offset_ != npos)
		{
			p[basename_offset_] = 0;
			parse();
		}
		else if (ends_with_sep_)
		{
			ends_with_sep_ = false;
			p[p.length()-1] = 0;
			parse();
		}
		return *this;
	}

	// Inserts text after basename and before extension
	basic_path_t& insert_basename_suffix(const char_type* new_suffix)
	{
		auto tmp = extension();
		if (has_extension())
			p[extension_offset_] = 0;
		p.append(new_suffix).append(tmp);
		parse();
		return *this;
	}

	basic_path_t& insert_basename_suffix(const string_type& new_suffix)
	{
		return insert_basename_suffix(new_suffix.c_str());
	}

	basic_path_t& remove_basename_suffix(const char_type* suffix)
	{
		if (has_basename() && suffix)
		{
			string_type s = suffix;
			char_type* ext = has_extension() ? &p[extension_offset_] : nullptr;
			if (ext) s += ext;
			char_type* suf = ext ? ext : &p[basename_offset_];
			while (*suf && suf < ext) suf++; // move to end of basename
			suf -= strlen(suffix);
			if (suf >= p.c_str())
			{
				if (!traits::compare(suf, s))
				{
					*suf = 0;
					p.append(ext);
					parse();
				}
			}
		}
	
		return *this;
	}

	basic_path_t& remove_basename_suffix(const string_type& suffix)
	{
		return remove_basename_suffix(suffix.c_str());
	}
	
	// decomposition
	const_array_ref c_str() const { return (const_array_ref)p; }
	operator const char_type*() const { return p; }
	operator string_type() const { return p; }

	#define NULL_STR_PIECE string_piece_type(p.c_str(), p.c_str())
	string_piece_type string_piece() const { return string_piece_type(p.c_str(), p.c_str() + length_); }
	string_piece_type root_path_piece() const { return has_root_name() ? (has_root_directory() ? string_piece_type(p.c_str(), p.c_str() + root_dir_offset_ + 1) : string_piece()) : (traits::has_vol(p) ? NULL_STR_PIECE : root_directory_piece()); }
	string_piece_type root_name_piece() const { return has_root_name_ ? (has_root_directory() ? string_piece_type(p.c_str(), p.c_str() + root_dir_offset_) : string_piece()) : NULL_STR_PIECE; }
	string_piece_type root_directory_piece() const { return has_root_directory() ? string_piece_type(p.c_str() + root_dir_offset_, p.c_str() + root_dir_offset_ + 1) : NULL_STR_PIECE; }
	string_piece_type relative_path_piece() const { const char* first = after_seps(p.c_str() + (has_root_directory() ? root_dir_offset_ + 1 : 0)); return string_piece_type(first, first + length_); }
	string_piece_type parent_path_piece() const { return has_parent_path() ? string_piece_type(p.c_str(), p.c_str() + parent_end_offset_) : NULL_STR_PIECE; }
	string_piece_type basename_piece() const { return has_basename() ? (string_piece_type(p.c_str() + basename_offset_, has_extension() ? (p.c_str() + extension_offset_) : (p.c_str() + length_))) : NULL_STR_PIECE; }
	string_piece_type filename_piece() const { return ends_with_sep_ ? string_piece_type(traits::dot_str(), traits::dot_str() + 1) : (basename_offset_ != npos ? string_piece_type(p.c_str() + basename_offset_, p.c_str() + length_) : string_piece()); }
	string_piece_type extension_piece() const { return has_extension() ? string_piece_type(p.c_str() + extension_offset_, p.c_str() + length_) : NULL_STR_PIECE; }
	#undef NULL_STR_PIECE

	basic_path_t string() const { string_piece_type sp = string_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t root_path() const { string_piece_type sp = root_path_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t root_name() const { string_piece_type sp = root_name_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t root_directory() const { string_piece_type sp = root_directory_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t relative_path() const { string_piece_type sp = relative_path_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t parent_path() const { string_piece_type sp = parent_path_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t basename() const { string_piece_type sp = basename_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t filename() const { string_piece_type sp = filename_piece(); return basic_path_t(sp.first, sp.second); }
	basic_path_t extension() const { string_piece_type sp = extension_piece(); return basic_path_t(sp.first, sp.second); }

	// All elements up to extension
	string_type stem() const { return has_extension() ? (has_basename() ? string_type(p.c_str(), p.c_str() + extension_offset_) : string_type()) : p; }
	
	// All elements up to and including the left-most colon
	string_type prefix() const { return traits::has_vol(p) ? string_type(p.c_str(), p.c_str() + 2) : string_type(); }

	// query
	/* constexpr */ size_type capacity() const { return Capacity; }
	bool empty() const { return p.empty(); }
	bool has_root_path() const { return has_root_directory() || has_root_name(); }
	bool has_root_name() const { return has_root_name_; }
	bool has_root_directory() const { return root_dir_offset_ != npos; }
	bool has_relative_path() const { return !p.empty() && has_root_directory() && !traits::is_sep(p[0]) && !p[1]; }
	bool has_parent_path() const { return parent_end_offset_ != 0 && parent_end_offset_ != npos; }
	bool has_filename() const { return has_basename() || has_extension(); }

	bool is_absolute() const { return (has_root_directory() && !traits::posix && has_root_name());  }
	bool is_windows_absolute() const { return is_absolute() || base_path_traits<char_type, false>::has_vol(p);  } // it is too annoying to live without this
	bool is_relative() const { return !is_absolute(); }
	bool is_windows_relative() const { return !is_windows_absolute(); }
	bool is_unc() const { return traits::is_unc(p); }
	bool is_windows_unc() const { return base_path_traits<char_type, false>::is_unc(p); }
	bool has_basename() const { return basename_offset_ != npos; }
	bool has_extension() const { return extension_offset_ != npos; }
	bool has_extension(const char_type* _Extension) const { return _Extension && traits::is_dot(*_Extension) && has_extension() ? !traits::compare(&p[extension_offset_], _Extension) : false; }

	hash_type hash() const { return hash_; }

	// Returns similar to strcmp/stricmp depending on path_traits. NOTE: A clean
	// and unclean path will not compare to be the same.
	int compare(const basic_path_t& that) const { return traits::compare(this->c_str(), that.c_str()); }
	int compare(const char_type* that) const { return traits::compare(this->c_str(), that); }

	basic_path_t relative_path(const char* root) const
	{
		basic_path_t rel;
		if (root && *root)
		{
			size_t cmn_len = cmnroot(p, root);
			if (cmn_len)
			{
				size_t nseps = 0;
				size_t index = cmn_len - 1;
				while (root[index])
				{
					if (traits::is_sep(root[index]) && 0 != root[index + 1])
						nseps++;
					index++;
				}
				for (size_t i = 0; i < nseps; i++)
					rel /= traits::dotdot_str();
			}

			const char* src = (*this).c_str() + cmn_len;
			if (traits::is_sep(*src))
				src++;
			rel /= src;
		}
		return rel;
	}

private:
	typedef unsigned short index_type;
	static const index_type npos = ~0u & 0xffff;
	unsigned short idx(const char_type* _PtrIntoP) { return _PtrIntoP ? static_cast<unsigned short>(std::distance((const char_type*)p, _PtrIntoP)) : npos; }

	hash_type hash_;
	string_type p;
	index_type root_dir_offset_; // index of first non-root-name slash
	index_type parent_end_offset_; // index of first right-most slash backed up past any redundant slashes
	index_type basename_offset_; // index of just after right-most slash
	index_type extension_offset_; // index of right-most dot
	index_type length_; // or offset of nul terminator
	bool has_root_name_;
	bool ends_with_sep_;

	static const char_type* after_seps(const char_type* p) { while (*p && traits::is_sep(*p)) p++; return p; }
	static const char_type* to_sep(const char_type* p) { while (*p && !traits::is_sep(*p)) p++; return p; }
	static const char_type* find_root_directory(const char_type* path, const char_type* root_name)
	{
		const char_type* dir = root_name;
		if (dir)
		{
			dir = after_seps(dir); // move past unc seps
			dir = to_sep(dir); // move past unc name
			if (!*dir) dir = nullptr; // if after-unc sep, then no rootdir
		}

		else if (traits::is_sep(path[0])) // rootdir is has_root_name() || is_sep(p[0])
			dir = path;
		return dir;
	}

	void parse()
	{
		if (traits::always_clean) clean_path(p, p);
		const char_type *rootname = nullptr, *path = nullptr, *parentend = nullptr, *base = nullptr, *ext = nullptr;
		length_ = static_cast<index_type>(tsplit_path<char_type>(p, traits::posix, &rootname, &path, &parentend, &base, &ext));
		has_root_name_ = !!rootname;
		basename_offset_ = idx(base);
		extension_offset_ = idx(ext);
		root_dir_offset_ = idx(find_root_directory(p, rootname));
		parent_end_offset_ = idx(parentend);
		ends_with_sep_ = length_ > 1 && traits::is_sep(p[length_-1]);
		hash_ = fnv1a<hash_type>(p);
	}
};

typedef basic_path_t<char> posix_path_t;
typedef basic_path_t<wchar_t> posix_wpath_t;

typedef basic_path_t<char, default_windows_path_traits<char>> windows_path_t;
typedef basic_path_t<wchar_t, default_windows_path_traits<wchar_t>> windows_wpath_t;

typedef posix_path_t path_t;
typedef posix_wpath_t wpath_t;

template<typename charT, typename traitsT>
char* to_string(char* dst, size_t dst_size, const basic_path_t<charT, traitsT>& value)
{
	try { ((const basic_path_t<charT, traitsT>::string_type&)value).copy_to(dst, dst_size); }
	catch (std::exception&) { return nullptr; }
	return dst;
}

template<typename charT, typename traitsT>
bool from_string(basic_path_t<charT, traitsT>* out_value, const char* src)
{
	try { *out_value = src; }
	catch (std::exception&) { return false; }
	return true;
}

}

namespace std {

template<typename charT, typename traitsT>
struct hash<ouro::basic_path_t<charT, traitsT>> { std::size_t operator()(const ouro::basic_path_t<charT, traitsT>& path) const { return path.hash(); } };

}
