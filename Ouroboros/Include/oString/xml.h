// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// xml document parser. Replaces delimiters inline with null terminators 
// and caches indices to the values

// todo: add separate support for <?> and <!> nodes_. Right now the parser skips
// over all <! nodes_ as comments and skips over all <?> nodes_ before the first
// non-<?> node. Thereafter nodes_ starting with ? will be parsed the same as
// regular nodes_, which isn't consistent/correct/standard.

// todo: add support for text fragments: multiple node values interspersed with
// child nodes_. Add a new handle "text" and have a first/next text.

#pragma once
#include <oString/fixed_string.h>
#include <oString/stringize.h>
#include <oString/text_document.h>
#include <cstdint>
#include <cstring>

// Convenience macros for iterating through a list of child nodes_ and attrs_
#define oString_xml_foreach_child(child__, xml__, parent__) for (ouro::xml::node (child__) = (xml__).first_child(parent__); (child__); (child__) = (xml__).next_sibling(child__))
#define oString_xml_foreach_attr(attr__, xml__, node__) for (ouro::xml::attr (attr__) = (xml__).first_attr(node__); (attr__); attr__ = (xml__).next_attr(attr__))

namespace ouro {

class xml
{
public:
	typedef uint32_t index_type;
	typedef char char_type;

	typedef struct node__ {}* node;
	typedef struct attr__ {}* attr;

	class visitor
	{
	public:
		// Used with the visit API. The XML document will be traversed depth-first
		// and events will be received in that order. NOTE: Currently only pre-text,
		// that is text that comes before a child node will be respected. This is a 
		// bug that will be fixed by calling an event for each section of text 
		// between child nodes_ - because of the parsing done by this object, a 
		// string of the entire block will never be returned. If any function 
		// returns false, traversal is aborted.

		typedef std::pair<const char_type*, const char_type*> attr_type; // name/value pair
		virtual bool node_begin(const char_type* node_name, const char_type* xref, const attr_type* attributes, size_t num_attributes) = 0;
		virtual bool node_end(const char_type* node_name, const char_type* xref) = 0;
		virtual bool node_text(const char_type* node_name, const char_type* xref, const char_type* _NodeText) = 0;
	};

	xml() : size_(0) {}
	xml(const char_type* uri, char_type* data, blob::deleter_fn deleter, size_t est_num_nodes_ = 100, size_t est_num_attrs_ = 500)
		: buffer_(uri, data, deleter)
	{
		size_ = sizeof(*this) + strlen(buffer_.c_str()) + 1;
		nodes_.reserve(est_num_nodes_);
		attrs_.reserve(est_num_attrs_);
		index_buffer();
		size_ += nodes_.capacity() * sizeof(index_type) + attrs_.capacity() * sizeof(index_type);
	}

	xml(const char_type* uri, const char_type* data, size_t est_num_nodes_ = 100, size_t est_num_attrs_ = 500)
	{
		size_t copy_size = strlen(data) + 1;
		char* copy = new char[copy_size];
		strlcpy(copy, data, copy_size);
		buffer_ = std::move(detail::text_buffer(uri, copy, copy_size, [](void* p) { delete p; }));
		size_ = sizeof(*this) + copy_size;
		nodes_.reserve(est_num_nodes_);
		attrs_.reserve(est_num_attrs_);
		index_buffer();
		size_ += nodes_.capacity() * sizeof(index_type) + attrs_.capacity() * sizeof(index_type);
	}

	xml(xml&& that) { operator=(std::move(that)); }
	xml& operator=(xml&& that)
	{
		if (this != &that)
		{
			buffer_ = std::move(that.buffer_);
			attrs_ = std::move(that.attrs_);
			nodes_ = std::move(that.nodes_);
			size_ = std::move(that.size_);
		}
		return *this;
	}

	inline operator bool() const { return (bool)buffer_; }
	inline const char_type* name() const { return buffer_.uri_; }
	inline size_t size() const { return size_; }

	// Node API
	inline node root() const { return node(1); } 
	inline node parent(node n) const { return node(uintptr_t(Node(n).up));  }
	inline node first_child(node parent_node) const { return node(uintptr_t(Node(parent_node).down)); }
	inline node next_sibling(node prior_sibling) const { return node(uintptr_t(Node(prior_sibling).next)); }
	inline const char_type* node_name(node n) const { return buffer_.c_str() + Node(n).name; }
	inline const char_type* node_value(node n) const { return buffer_.c_str() + Node(n).value; }
	
	// Attribute API
	inline attr first_attr(node n) const { return attr(uintptr_t(Node(n).Attr)); }
	inline attr next_attr(attr a) const { return Attr(a).name ? attr(a + 1) : 0; }
	inline const char_type* attr_name(attr a) const { return buffer_.c_str() + Attr(a).name; }
	inline const char_type* attr_value(attr a) const { return buffer_.c_str() + Attr(a).value; }
	
	// Convenience functions that use the above API
	inline node first_child(node parent_node, const char_type* _node_name) const
	{
		node n = first_child(parent_node);
		while (n && _stricmp(node_name(n), _node_name))
			n = next_sibling(n);
		return n;
	}

	inline node next_sibling(node prior_sibling, const char_type* _node_name) const
	{
		node n = next_sibling(prior_sibling);
		while (n && _stricmp(node_name(n), _node_name))
			n = next_sibling(n);
		return n;
	}

	inline attr find_attr(node n, const char_type* attr_name_) const
	{
		for (attr a = first_attr(n); a; a = next_attr(a))
			if (!_stricmp(attr_name(a), attr_name_))
				return a;
		return attr(0);
	}

	inline const char_type* find_attr_value(node n, const char_type* attr_name_) const
	{
		attr a = find_attr(n, attr_name_);
		return a ? attr_value(a) : nullptr;
	}
	
	template<typename T> bool attr_value(attr a, T* out_value) const
	{
		return from_string(out_value, attr_value(a));
	}
	
	template<typename charT, size_t capacity> 
	bool attr_value(attr a, fixed_string<charT, capacity>& out_value) const
	{
		out_value = attr_value(a);
		return true;
	}

	template<typename T> bool find_attr_value(node n, const char_type* attr_name_, T* out_value) const
	{
		attr a = find_attr(n, attr_name_);
		return a ? from_string(out_value, attr_value(a)) : false;
	}

	template<typename charT, size_t capacity>
	bool find_attr_value(node n, const char_type* attr_name_, fixed_string<charT, capacity>& out_value) const
	{
		attr a = find_attr(n, attr_name_);
		if (a) out_value = attr_value(a);
		return !!a;
	}

	inline bool visit(visitor& visitor) const
	{
		detail::text_buffer::std_vector<visitor::attr_type> a;
		a.reserve(16);
		return visit(first_child(root()), 0, a, visitor);
	}

	// Helper function to be called from inside visitor functions
	static const char_type* find_attr_value(const visitor::attr_type* attrs_, size_t num_attrs_, const char_type* attr_name_)
	{
		for (size_t i = 0; i < num_attrs_; i++)
			if (!_stricmp(attrs_[i].first, attr_name_))
				return attrs_[i].second;
		return nullptr;
	}

	// Helper function to be called from inside visitor functions
	template<typename T>
	static bool find_attr_value(const visitor::attr_type* attrs_, size_t num_attrs_, const char_type* attr_name_, T* out_value)
	{
		const char_type* v = find_attr_value(attrs_, num_attrs_, attr_name_);
		return v ? from_string(out_value, v) : false;
	}

	template<typename charT, size_t capacity>
	static bool find_attr_value(const visitor::attr_type* attrs_, size_t num_attrs_, const char_type* attr_name_, fixed_string<charT, capacity>& out_value)
	{
		const char_type* v = find_attr_value(attrs_, num_attrs_, attr_name_);
		if (v) out_value = v;
		return !!v;
	}

	// It is common that ids for nodes_ are unique to the file, so find the 
	// specified string in the id or name attribute of the first node in depth-
	// first order.
	inline node find_id(const char_type* id) const { return find_id(root(), id); }

	// xref
	// This is like xpointer, but not as robust or verbose. If an xref does not 
	// begin with a '/', then this follows the typical convention of expecting 
	// that there is exactly one node with an id or name attribute that has the 
	// value and this returns the first match as found by a depth-first
	// search. If there is a slash, then this is an xref path. It is '/' delimited
	// and each section can either be the value of an id or name attr (id is looked
	// for first, then if not there then name is looked for) or it can be a 1-based
	// index of the sibling nodes_.
	inline char_type* make_xref(char_type* dst, size_t dst_size_, node n) const
	{
		detail::text_buffer::std_vector<node> path;
		path.reserve(16);
		
		path.push_back(n);
		node p = parent(n);
		while (p != root()) // while not the root or invalid
		{
			path.push_back(p);
			p = parent(p);
		}

		if (path.empty())
		{
			if (dst_size_ < 2) return nullptr;
			dst[0] = '/';
			dst[1] = '\0';
			return dst;
		}
		else
			*dst = '\0';

		for (auto it = path.rbegin(); it != path.rend(); ++it)
		{
			size_t len = strlcat(dst, "/", dst_size_);
			if (len >= dst_size_) return nullptr;
			
			const char_type* id = get_id(*it);
			char_type buf[10];
			if (!id)
			{
				int SiblingIndex = find_sibling_offset(*it) + 1;
				to_string(buf, SiblingIndex);
				id = buf;
			}

			len = strlcat(dst, id, dst_size_);
			if (len >= dst_size_) return nullptr;
		}

		return dst;
	}

	template<size_t size_>
	char_type* make_xref(char_type (&dst)[size_], node n) const { return make_xref(dst, size_, n); }

	template<typename charT, size_t capacity> 
	char_type* make_xref(fixed_string<charT, capacity>& dst, node n) const { return make_xref(dst, dst.capacity(), n); }

	node find_xref(const char_type* xref) const
	{
		if (!xref || !xref[0]) return node(0);
		if (xref[0] == '/')
		{
			if (xref[1] == '\0') return root();
			return find_xref(first_child(root()), xref+1);
		}
		return find_id(root(), xref);
	}

private:
	detail::text_buffer buffer_;
	
	struct ATTR
	{
		ATTR() : name(0), value(0) {}
		index_type name, value;
	};

	struct NODE
	{
		NODE() : next(0), up(0), down(0), Attr(0), name(0), value(0) {}
		index_type next, up, down, Attr, name, value;
	};

	typedef detail::text_buffer::std_vector<ATTR> attrs_t;
	typedef detail::text_buffer::std_vector<NODE> nodes_t;

	attrs_t attrs_;
	nodes_t nodes_;
	size_t size_;

	inline const ATTR& Attr(attr a) const { return attrs_[(size_t)a]; }
	inline const NODE& Node(node n) const { return nodes_[(size_t)n]; }

	inline ATTR& Attr(attr a) { return attrs_[(size_t)a]; }
	inline NODE& Node(node n) { return nodes_[(size_t)n]; }

	// Parsing functions
	static inline ATTR make_attr(char_type* xml_start, char_type*& xml_current, bool& at_end);
	inline void index_buffer();
	inline void make_node_attrs_(char_type* xml_start, char_type*& _xml, NODE& n);
	inline node make_next_node(char_type*& _xml, node parent_node, node previous, int& open_tag_count, int& close_tag_count);
	inline void make_next_node_children(char_type*& _xml, node parent_node, int& open_tag_count, int& close_tag_count);

	inline bool visit(node n, int sibling_index, detail::text_buffer::std_vector<visitor::attr_type>& attrs, visitor& visitor) const
	{
		const char_type* nname = node_name(n);
		const char_type* nval = node_value(n);
		attrs.clear();
		for (attr a = first_attr(n); a; a = next_attr(a))
			attrs.push_back(visitor::attr_type(attr_name(a), attr_value(a)));

		mstring xref;
		make_xref(xref, n);

		if (!visitor.node_begin(nname, xref, attrs.data(), attrs.size()))
			return false;

		// note: this should become interspersed with children for cases like this:
		// <node>
		//   some text
		//   <child />
		//   more text
		//   <child />
		//   final text
		// </node>
		if (nval && nval[0] && !visitor.node_text(nname, xref, nval))
			return false;

		int i = 0;
		for (node c = first_child(n); c; c = next_sibling(c), i++)
			if (!visit(c, i, attrs, visitor))
				return false;

		if (!visitor.node_end(nname, xref))
			return false;

		return true;
	}

	inline const char_type* get_id(node n) const
	{
		const char_type* id = find_attr_value(n, "id");
		if (!id) id = find_attr_value(n, "name");
		return id;
	}

	inline node find_id(node n, const char_type* id) const
	{
		const char_type* id_ = get_id(n);
		if (!_stricmp(id_, id))
			return n;

		for (node c = first_child(n); c; c = next_sibling(c))
		{
			node nd = find_id(c, id);
			if (nd) return nd;
		}

		return node(0);
	}

	// Return the 0-based offset from the first child of this node's parent.
	inline int find_sibling_offset(node n) const
	{
		int offset = 0;
		if (n)
		{
			node p = parent(n);
			node nd = first_child(p);
			while (nd != n)
			{
				offset++;
				nd = next_sibling(nd);
			}
		}
		return offset;
	}

	inline node find_xref(node n, const char_type* xref) const
	{
		sstring tmp;
		const char_type* slash = strchr(xref, '/');
		if (slash)
			tmp.assign(xref, slash);
		else 
			tmp = xref;

		node nd = n;
		int Index = -1;
		if (from_string(&Index, tmp))
		{ // by index
			int i = 1;
			while (nd)
			{
				if (Index == i)
					break;
				i++;
				nd = next_sibling(nd);
			}
		}

		else
		{ // by name
			while (nd)
			{
				const char_type* id = get_id(nd);
				if (id && !_stricmp(id, tmp))
					break;
				nd = next_sibling(nd);
			}
		}

		if (nd)
		{
			if (slash)
				return find_xref(first_child(nd), slash+1);
			return nd;
		}
		return node(0);
	}
};

	namespace detail
	{
		// _xml must be pointing at a '<' char_type. This will leave _xml point at a '<'
		// to a non-reserved node.
		template<typename charT>
		inline void skip_reserved_nodes_(charT*& _xml)
		{
			bool skipping = false;
			do
			{
				skipping = false;
				if (*(_xml+1) == '!') // skip comment
				{
					skipping = true;
					_xml = strstr(_xml+4, "-->");
					if (!_xml) throw new text_document_error(text_document_errc::unclosed_scope);
					_xml += strcspn(_xml, "<");
				}
				if (*(_xml+1) == '?') // skip meta tag
				{
					skipping = true;
					_xml += strcspn(_xml+2, ">");
					_xml += strcspn(_xml, "<");
				}
			} while (skipping);
		}

		template<typename charT>
		inline void ampersand_decode(charT* s)
		{
			static const struct { const char* code; unsigned char len; char c; } reserved[] = { { "lt;", 3, '<' }, { "gt;", 3, '>' }, { "amp;", 4, '&' }, {"apos;", 5, '\'' }, { "quot;", 5, '\"' }, };

			s = strchr(s, '&'); // find encoding marker
			if (s)
			{
				charT* w = s; // set up a write head to write in-place
				while (*s)
				{
					if (*s == '&' && ++s) // process a full symbol at a time
					{
						for (const auto& sym : reserved)
						{
							if (!memcmp(sym.code, s, sym.len)) // if this is an encoding, decode
							{
								*w++ = sym.c;
								s += sym.len;
								break;
							}
						}
					}
					else
						*w++ = *s++; // copy character normally
				}
				*w = 0; // ensure the final string is nul-terminated
			}
		}
	} // namespace detail

void xml::index_buffer()
{
	char_type* start = buffer_.c_str() + strcspn(buffer_.c_str(), "<"); // find first opening tag
	// offsets of 0 must point to the empty string, so assign first byte as empty
	// string... but ensure we've moved past where it's important to have that
	// char_type be meaningful.
	if (start == buffer_.c_str()) detail::skip_reserved_nodes_(start);
	attrs_.push_back(ATTR()); // init 0 to 0 offset from start
	nodes_.push_back(NODE()); // init 0 to point to 0 offset from start
	nodes_.push_back(NODE()); // init 1 to be the root - the parent of the first node
	int OpenTagCount = 0, CloseTagCount = 0; // these should be equal by the end
	make_next_node_children(start, root(), OpenTagCount, CloseTagCount); // start recursing
	*buffer_.c_str() = 0; // make the first char_type nul so 0 offsets are the empty string
	if (OpenTagCount != CloseTagCount) throw text_document_error(text_document_errc::unclosed_scope); // if not equal, something went wrong
}

// This expects xml_current to be pointing after a node name and before an
// attribute's name. This can end with false if a new attribute could not be
// found and thus xml_current will be pointing to either a '/' for the one-line
// node definition (<mynode attr="val" />) or a '>' for the multi-line node
// definition (<mynode attr="val"></mynode>).
xml::ATTR xml::make_attr(xml::char_type* xml_start, xml::char_type*& xml_current, bool& at_end)
{
	ATTR a;
	while (*xml_current && !strchr("_/>", *xml_current) && !isalnum(*xml_current)) xml_current++; // move to next identifier ([_0-9a-zA-Z), but not an end tag ("/>" or ">")
	if (*xml_current != '>' && *xml_current != '/') // if not at the end of the node
	{
		a.name = static_cast<index_type>(std::distance(xml_start, xml_current)); // assign the offset to the start of the name
		xml_current += strcspn(xml_current, "/> \t\r\n="); // move to end of identifier 
		const char_type* checkEq = xml_current + strspn(xml_current, " \t\r\n"); // move to next non-whitespace

		if (*checkEq == '=')
		{
			*xml_current++ = 0;
			xml_current += strcspn(xml_current, "\"'"); // move to start of value
			a.value = static_cast<index_type>(std::distance(xml_start, ++xml_current)); // assign the offset to the start of the value (past quote)
			xml_current += strcspn(xml_current, "\"'"), *xml_current++ = 0; // find closing quote and nul-terminate
			detail::ampersand_decode(xml_start + a.name), detail::ampersand_decode(xml_start + a.value); // decode ampersand-encoding for the values (always makes string shorter, so this is ok to do inline)
		}

		else
		{
			if (*checkEq == '>' || *checkEq == '/')
				at_end = true;
			*xml_current++ = 0;
			a.value = a.name; 
		}
	}
	return a;
}

void xml::make_node_attrs_(char_type* xml_start, char_type*& _xml, NODE& n)
{
	n.Attr = static_cast<index_type>(attrs_.size());
	bool AtEnd = false;
	while (*_xml != '>' && *_xml != '/' && !AtEnd)
	{
		ATTR a = make_attr(xml_start, _xml, AtEnd);
		if (a.name)
			attrs_.push_back(a);
	}
	if (*_xml == '>') _xml++;
	(n.Attr == attrs_.size()) ? n.Attr = 0 : attrs_.push_back(ATTR()); // either null the list if no elements added, or push a null terminator
}

void xml::make_next_node_children(char_type*& _xml, node parent_node, int& open_tag_count, int& close_tag_count)
{
	node prev = 0;
	while (*_xml != 0 && *(_xml+1) != '/' && *(_xml+1) != 0) // Check for end of the buffer as well
	{
		if (*(_xml+1) == '!') detail::skip_reserved_nodes_(_xml);
		else prev = make_next_node(_xml, parent_node, prev, open_tag_count, close_tag_count);
	}
	if (*(_xml+1) == '/') // Validate this is an end tag
		close_tag_count++;
	if (_xml[0] && _xml[1] && _xml[2])
	{
		_xml += strcspn(_xml+2, ">") + 1;
		_xml += strcspn(_xml, "<");
	}
}

xml::node xml::make_next_node(char_type*& _xml, node parent_node, node previous, int& open_tag_count, int& close_tag_count)
{
	open_tag_count++;
	NODE n;
	n.up = (index_type)(uintptr_t)parent_node;
	if (parent_node || *_xml == '<') detail::skip_reserved_nodes_(_xml); // base-case where the first char_type of file is '<' and that got nulled for the 0 offset empty value
	n.name = static_cast<index_type>(std::distance(buffer_.c_str(), ++_xml));
	_xml += strcspn(_xml, " /\t\r\n>");
	bool veryEarlyOut = *_xml == '/';
	bool process = *_xml != '>' && !veryEarlyOut;
	*_xml++ = 0;
	if (process) make_node_attrs_(buffer_.c_str(), _xml, n);
	if (!veryEarlyOut)
	{
		if (*_xml != '/' && *_xml != '<') n.value = static_cast<index_type>(std::distance(buffer_.c_str(), _xml++)), _xml += strcspn(_xml, "<");
		if (*(_xml+1) == '/') *_xml++ = 0;
		else n.value = 0;
	}
	else n.value = 0;
	index_type newNode = static_cast<index_type>(nodes_.size());
	if (previous) Node(previous).next = newNode;
	else if (parent_node) Node(parent_node).down = newNode; // only assign down for first child - all other siblings don't reassign their parent's down.
	nodes_.push_back(n);
	detail::ampersand_decode(buffer_.c_str() + n.name), detail::ampersand_decode(buffer_.c_str() + n.value);
	if (!veryEarlyOut && *_xml != '/') // recurse on children nodes_
	{
		node p = node((uintptr_t)newNode);
		make_next_node_children(_xml, p, open_tag_count, close_tag_count);
	}
	else 
	{
		close_tag_count++;
		_xml += strcspn(_xml, "<");
	}
	return node((uintptr_t)newNode);
}

}
