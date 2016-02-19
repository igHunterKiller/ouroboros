// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oBase/osc.h>
#include <oCore/assert.h>
#include <oCore/byte.h>
#include <oCore/endian.h>
#include <oString/string.h>

#define oUNKNOWN_SIZE SIZE_MAX

#define oASSERT_ALIGNED(x) oAssert(aligned(x, 4), "The destination pointer must be 4-byte aligned");

namespace ouro { namespace osc {

// If a nullptr is serialized as a string, we still need to identify that the
// occurance happened, so write a special character.
static const char NULL_STRING = -1;

// Field alignment is in terms of offset from base struct pointer, not absolute
// memory. This is really apparent when there are double-word types, such as 
// long longs on 32-bit builds.
void* move_to_next_field(const void* struct_base, const void* last_field_end_unaligned, size_t next_field_alignment)
{
	ptrdiff_t offset = ptrdiff_t((const uint8_t*)last_field_end_unaligned - (const uint8_t*)struct_base);
	return byte_add((void*)struct_base, align(offset, next_field_alignment));
}

size_t SizeofFixedString(int type)
{
	return (type - '0') * 64 * sizeof(char);
}

template<typename ptr_t>
struct Visitor
{
	typedef std::function<void(int type, ptr_t field, size_t field_size)> Fn;
};

namespace STRUCT {

// STRUCT* Aligns to the size of type and visits data at that address and then
// returns a pointer just after the read field

template<typename ptr_t>
static ptr_t visit_next_intrinsic(int type, ptr_t struct_base, ptr_t field, size_t field_size, const typename Visitor<ptr_t>::Fn& visitor)
{
	void* p = move_to_next_field(struct_base, field, field_size);
	visitor(type, p, field_size);
	return byte_add(p, field_size);
}

template<typename ptr_t>
static ptr_t visit_next_char(ptr_t struct_base, ptr_t field, const typename Visitor<ptr_t>::Fn& visitor)
{
	visitor('c', field, sizeof(char));
	return byte_add(field, sizeof(char));
}

template<typename ptr_t>
static ptr_t visit_next_string(ptr_t struct_base, ptr_t field, const typename Visitor<ptr_t>::Fn& visitor)
{
	void* p = move_to_next_field(struct_base, field, sizeof(const char*));
	const char* s = *(const char**)p;
	visitor('s', p, s ? (strlen(s)+1) : sizeof(NULL_STRING));
	return byte_add(p, sizeof(ptr_t));
}

template<typename ptr_t>
static ptr_t visit_next_fixed_string(int type, ptr_t struct_base, ptr_t field, const typename Visitor<ptr_t>::Fn& visitor)
{
	void* p = move_to_next_field(struct_base, field, sizeof(char));
	visitor(type, p, strlen((const char*)p)+1);
	return byte_add(p, SizeofFixedString(type));
}

template<typename ptr_t>
static ptr_t visit_next_blob(ptr_t struct_base, ptr_t field, const typename Visitor<ptr_t>::Fn& visitor)
{
	void* p = move_to_next_field(struct_base, field, sizeof(int));
	int size = *(int*)p;
	p = move_to_next_field(struct_base, byte_add(p, sizeof(int)), sizeof(ptr_t));
	visitor('b', *(void**)p, size);
	return byte_add(p, sizeof(ptr_t));
}

} // namespace STRUCT

template<typename ptr_t>
bool visit_struct_fields_internal(const typename Visitor<ptr_t>::Fn& visitor, const char* typetags, ptr_t _struct, size_t struct_size, char* optional_patch_tags = nullptr)
{
	if (!visitor || !typetags || !_struct || !struct_size)
		oThrow(std::errc::invalid_argument, "null value");

	if (*typetags != ',')
		oThrow(std::errc::invalid_argument, "TypeTags must start with a ',' character");

	auto tag = typetags;
	auto patchTag = optional_patch_tags;
	ptr_t p = _struct;
	auto end = byte_add(p, struct_size);

	while (*(++tag))
	{
		if (p >= end)
		{
			if (p == end && (*tag == 0 || *tag == '[' || *tag == ']'))  // If we finish on any of these tags it's ok since they do not expect valid data
				return true;

			oThrow(std::errc::invalid_argument, "Tag string \"%s\" has run past the size of the struct pointed to by pointer 0x%p", typetags, _struct);
		}

		if (patchTag)
			++patchTag;

		switch (*tag)
		{
			case 'r': case 'i': case 'f': p = STRUCT::visit_next_intrinsic(*tag, _struct, p, 4, visitor); break;
			case 'h': case 't': case 'd': p = STRUCT::visit_next_intrinsic(*tag, _struct, p, 8, visitor); break;
			case 'c': p = STRUCT::visit_next_char(_struct, p, visitor); break;
			case 's': p = STRUCT::visit_next_string(_struct, p, visitor); break;
			case 'b': p = STRUCT::visit_next_blob(_struct, p, visitor); break;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': p = STRUCT::visit_next_fixed_string(*tag, _struct, p, visitor); break;
			case 'T': case 'F': 
			{
				// Special case for boolean types
				bool* pBTag = (bool*)p;
				int bits = 0;
				while(*tag == 'T' || *tag == 'F')
				{
					visitor(*tag, nullptr, 0);

					if (patchTag)
					{
						*patchTag = *pBTag ? 'T' : 'F';
						++patchTag;
					}

					++bits;
					++tag;
					++pBTag;
					p = byte_add(p, 1);
				}
				// Offset number of bytes plus padding
				//p = byte_add(p, (bits / 8) + 1);

				// Back the tags up so we increment correctly in the next pass
				--tag; 
				if (patchTag)
					--patchTag;
			}
			break;
			default: visitor(*tag, nullptr, 0); break;
		}
	}

	return true;
}

bool visit_struct_fields(const char* typetags, void* _struct, size_t struct_size, const visitor_fn& visitor)
{
	return visit_struct_fields_internal<void*>(visitor, typetags, _struct, struct_size, nullptr);
}

bool visit_struct_fields(const char* typetags, const void* _struct, size_t struct_size, const visitor_const_fn& visitor)
{
	return visit_struct_fields_internal<const void*>(visitor, typetags, _struct, struct_size, nullptr);
}

namespace TAG {
	// TAG functions assume 4-byte alignment, assumes char is stored as an int and
	// does endian swapping on the data before visiting.

template<typename T, typename ptr_t, typename fn_t> 
static ptr_t visit_next_intrinsic(int type, ptr_t _pArgument, const fn_t& visitor)
{
	T swapped = to_big_endian(*(const T*)_pArgument);
	visitor(type, &swapped, sizeof(T));
	return byte_add(_pArgument, sizeof(T));
}

template<typename ptr_t, typename fn_t>
static ptr_t visit_next_char(ptr_t _pArgument, const fn_t& visitor)
{
	int cAsInt = to_big_endian(*(const int*)_pArgument);
	char c = (char)cAsInt;
	visitor('c', &c, sizeof(char)); // be honest about data and describe as a char
	return byte_add(_pArgument, sizeof(int)); // move past the original int in the buffer
}

template<typename ptr_t, typename fn_t>
static ptr_t visit_next_string(int type, ptr_t _pArgument, const fn_t& visitor)
{
	size_t size = strlen((const char*)_pArgument) + 1;
	visitor(type, _pArgument, size);
	return byte_add(_pArgument, align(size, 4));
}

template<typename ptr_t, typename fn_t>
static ptr_t visit_next_blob(ptr_t _pArgument, const fn_t& visitor)
{
	int size = to_big_endian(*(const int*)_pArgument);
	ptr_t p = byte_add(_pArgument, sizeof(int));
	visitor('b', p, size);
	return byte_add(p, align(size, 4));
}

} // namespace TAG

template<typename ptr_t, typename fn_t>
bool visit_msg_type_tags_internal(const char* typetags, ptr_t msg_args, fn_t visitor)
{
	if (!visitor || !typetags || *typetags != ',' || !msg_args)
		oThrow(std::errc::invalid_argument, "valid typetags must be specified that starts with ','");

	auto tag = typetags;
	auto p = msg_args;
	while (*(++tag))
	{
		switch (*tag)
		{
			case 'r': case 'i': case 'f': p = TAG::visit_next_intrinsic<int>(*tag, p, visitor); break;
			case 'h': case 't': case 'd': p = TAG::visit_next_intrinsic<long long>(*tag, p, visitor); break;
			case 'c': p = TAG::visit_next_char(p, visitor); break;
			case 'b': p = TAG::visit_next_blob(p, visitor); break;
			case 's': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': p = TAG::visit_next_string(*tag, p, visitor); break;
			default: visitor(*tag, nullptr, 0); break;
		}
	}

	return true;
}

bool visit_msg_type_tags(const char* typetags, const void* msg_args, const visitor_const_fn& visitor)
{
	return visit_msg_type_tags_internal(typetags, msg_args, visitor);
}

bool visit_msg_type_tags(const char* typetags, void* msg_args, const visitor_fn& visitor)
{
	return visit_msg_type_tags_internal(typetags, msg_args, visitor);
}

static void sum_field_sizes(int type, const void* field, size_t field_size, size_t* out_size_sum)
{
	*out_size_sum += align(field_size, 4);
	if (type == 'b') *out_size_sum += 4; // for the size item
}

size_t calc_num_fields(const char* typetags)
{
	bool InArray = false;
	size_t nFields = 0;
	const char* tag = typetags;
	while (*(++tag))
	{
		if (*tag == '[')
			InArray = true;
		else if (*tag == ']')
		{
			nFields++;
			InArray = false;
		}

		if (!InArray)
		{
			static const char* countedTags = "rifhtdcsb123456789P";
			if (strchr(countedTags, *tag))
				nFields++;
		}
	}

	return nFields;
}

size_t calc_args_data_size(const char* typetags, const void* _struct, size_t struct_size)
{
	size_t size = 0;
	if (!visit_struct_fields(typetags, _struct, struct_size, std::bind(sum_field_sizes, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, &size)))
		oThrow(std::errc::invalid_argument, "");
	return size;
}

size_t calc_msg_size(const char* address, const char* typetags, size_t _ArgumentsDataSize)
{
	size_t size = align(strlen(address) + 1, 4);
	if (typetags)
		size += align(strlen(typetags) + 1, 4);
	return size + _ArgumentsDataSize;
}

size_t calc_bundle_size(size_t _NumSubbundles, size_t _SumOfAllSubbundleSizes)
{	// sizeof "#bundle\0" + sizeof time-tag + a size per bundle + size of all bundles
	return 8 + 8 + (sizeof(int) * _NumSubbundles) + _SumOfAllSubbundleSizes;
}

template<typename T>
void AlignedIncrement(size_t* pSize)
{
	*pSize = align(*pSize, sizeof(T)) + sizeof(T);
}

size_t calc_deserialized_struct_size(const char* typetags)
{
	if (!typetags || *typetags != ',')
		oThrow(std::errc::invalid_argument, "valid typetags must be specified that starts with ','");

	size_t size = 0;
	const char* tag = typetags;
	while (*(++tag))
	{
		switch(*tag)
		{
			case 'r': case 'i': case 'f': AlignedIncrement<int>(&size); break;
			case 'h': case 't': case 'd': AlignedIncrement<long long>(&size); break;
			case 'c': AlignedIncrement<char>(&size); break;
			case 'b': AlignedIncrement<int>(&size); AlignedIncrement<void*>(&size); break;
			case 's': AlignedIncrement<void*>(&size); break;
			case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': size += atoi(tag) * 64; break;
			case 'T': case 'F': 
			{
					int bits = 1;
					while(tag[1] == 'T' || tag[1] == 'F')
				{
					++bits;
					++tag;
				}
				size += (bits / 8) + 1;
			}
		}
	}

	return align(size, sizeof(int));
}
const char* get_msg_address(const void* msg)
{
	return *(const char*)msg == '/' ? (const char*)msg : nullptr;
}

const char* get_msg_type_tags(const void* msg)
{
	const char* Address = get_msg_address(msg);
	if (Address)
	{
		const char* TypeTags = byte_add(Address, align(strlen(Address)+1, 4));
		return *TypeTags == ',' ? TypeTags : nullptr;
	}
	
	return nullptr;
}

namespace SERIALIZE {

template<typename T> static void* next_intrinsic(const void* struct_base, void* dst, const void* last_field_end_unaligned)
{
	oASSERT_ALIGNED(dst);
	const void* s = move_to_next_field(struct_base, last_field_end_unaligned, std::alignment_of<T>::value);
	*(T*)dst = to_big_endian(*(const T*)s);
	return byte_add(dst, sizeof(T));
}

static void* NextChar(void* dst, const void* src)
{
	oASSERT_ALIGNED(dst);
	int cAsInt = *(const char*)src;
	*(int*)dst = to_big_endian(cAsInt);
	return byte_add(dst, sizeof(int));
}

static void* copy_next_buffer(void* dst, size_t dst_size, const void* buf, size_t buf_size)
{
	oASSERT_ALIGNED(dst);
	oAssert(dst_size >= align(buf_size, 4), "");
	oAssert(buf, "A valid buffer must be specified");
	memcpy(dst, buf, buf_size);
	// pad with zeros out to 4-byte alignment
	char* p = (char*)byte_add(dst, buf_size);
	char* pend = align(p, 4);
	while (p < pend)
		*p++ = 0;
	return p;
}

static void* next_string(void* dst, size_t dst_size, const char* _String)
{
	oASSERT_ALIGNED(dst);
	if (_String)
		return copy_next_buffer(dst, dst_size, _String, strlen(_String) + 1);
	return copy_next_buffer(dst, dst_size, &NULL_STRING, 1);
}

static void* next_blob(void* dst, size_t dst_size, const void* src, size_t src_size)
{
	oASSERT_ALIGNED(dst);
	*(int*)dst = to_big_endian((int)src_size);
	return copy_next_buffer(byte_add(dst, sizeof(int)), oUNKNOWN_SIZE, src, src_size);
}

} // namespace SERIALIZE

static void serializer(int type, const void* field, size_t field_size, const void* out_struct_base, void** ppDest, void* pDstEnd)
{
	oAssert(*ppDest < pDstEnd, "Writing past end of buffer");
	switch (type)
	{
		case 'r': case 'i': case 'f': *ppDest = SERIALIZE::next_intrinsic<int>(out_struct_base, *ppDest, field); break;
		case 'h': case 't': case 'd': *ppDest = SERIALIZE::next_intrinsic<long long>(out_struct_base, *ppDest, field); break;
		case 'c': *ppDest = SERIALIZE::NextChar(*ppDest, field); break;
		case 's': *ppDest = SERIALIZE::next_string(*ppDest, oUNKNOWN_SIZE, *(const char**)field); break; // oUNKNOWN_SIZE is unsafe, ignoring buffer boundaries but this should already be allocated correctly
		case 'b': *ppDest = SERIALIZE::next_blob(*ppDest, oUNKNOWN_SIZE, field, field_size); break;
		case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': *ppDest = SERIALIZE::next_string(*ppDest, oUNKNOWN_SIZE, (const char*)field);
		default: break;
	}
}

namespace DESERIALIZE {

template<typename T> static void* next_intrinsic(void* struct_base, void* dst, const void* src)
{
	void* p = move_to_next_field(struct_base, dst, std::alignment_of<T>::value);
	*(T*)p = *(const T*)src;
	return byte_add(p, sizeof(T));
}

static void* next_string(void* struct_base, void* dst, const char* _String)
{
	// assign pointer into message buffer
	const char** s = (const char**)move_to_next_field(struct_base, dst, std::alignment_of<const char**>::value);
	*s = *(const char*)_String == NULL_STRING ? nullptr : (const char*)_String;
	return byte_add(s, sizeof(const char*));
}

static void* next_fixed_string(void* struct_base, void* dst, const char* _String, size_t _NumChars)
{
	// assign pointer into message buffer
	char* s = (char*)move_to_next_field(struct_base, dst, std::alignment_of<char>::value);
	strlcpy(s, _String, _NumChars);
	return byte_add(s, _NumChars * sizeof(char));
}

static void* next_blob(void* struct_base, void* dst, const void* src, size_t _SizeofSource)
{
	int* p = (int*)move_to_next_field(struct_base, dst, std::alignment_of<int>::value);
	*p = (int)_SizeofSource;
	p = (int*)move_to_next_field(struct_base, byte_add(p, sizeof(int)), std::alignment_of<void*>::value);
	*(const void**)p = src;
	return byte_add(p, sizeof(const void*));
}

} // namespace DESERIALIZE

static void deserializer(int type, const void* field, size_t field_size, void* out_struct_base, void** ppDest, void* pDstEnd)
{
	oAssert(*ppDest < pDstEnd || type == '[' || type == ']', "Writing past end of buffer");
	if ('T' != type && 'F' != type)
		*ppDest = align(*ppDest, sizeof(char));

	switch (type)
	{
		case 'r': case 'i': case 'f': *ppDest = DESERIALIZE::next_intrinsic<int>(out_struct_base, *ppDest, field); break;
		case 'h': case 't': case 'd': *ppDest = DESERIALIZE::next_intrinsic<long long>(out_struct_base, *ppDest, field); break;
		case 'c': *ppDest = DESERIALIZE::next_intrinsic<char>(out_struct_base, *ppDest, field); break;
		case 's': *ppDest = DESERIALIZE::next_string(out_struct_base, *ppDest, (const char*)field); break;
		case 'b': *ppDest = DESERIALIZE::next_blob(out_struct_base, *ppDest, field, field_size); break;
		case 'T': *(bool*)(*ppDest) = true; (*ppDest) = (bool*)(*ppDest) + 1; break;
		case 'F': *(bool*)(*ppDest) = false; (*ppDest) = (bool*)(*ppDest) + 1; break;
		case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': *ppDest = DESERIALIZE::next_fixed_string(out_struct_base, *ppDest, (const char*)field, SizeofFixedString(type)); break;
		default: break;
	}
}

size_t serialize_struct_to_msg(const char* address, const char* typetags, const void* _struct, size_t struct_size, void* msg, size_t msg_size)
{
	if (!address || *address != '/' || !msg || !_struct || struct_size == 0)
		oThrow(std::errc::invalid_argument, "");

	void* p = msg;
	void* pend = byte_add(p, msg_size);

	p = SERIALIZE::next_string(p, msg_size, address);
	char* pPatchTag = (char*)p;
	p = SERIALIZE::next_string(p, size_t((uint8_t*)pend - (uint8_t*)p), typetags);

	size_t szSerializedMessage = 0;
	oAssert(size_t((uint8_t*)pend - (uint8_t*)p) >= calc_args_data_size(typetags, _struct, struct_size), "");
	if (!visit_struct_fields_internal(std::bind(serializer, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, _struct, &p, pend), typetags, _struct, struct_size, pPatchTag))
		return 0;

	return (char*)p - (char*)msg;
}

bool deserialize_msg_to_struct(const void* msg, void* _struct, size_t struct_size)
{
	// if struct ptr is not aligned, all the alignment logic will be off
	oCheck(msg && aligned(_struct, 4), std::errc::invalid_argument, "");

	const char* tags = get_msg_type_tags(msg);
	oCheck(tags, std::errc::protocol_error, "failed to read message type tags");

	const void* args = (const void*)byte_add(tags, align(strlen(tags)+1, 4));
	void* p = _struct;
	void* pend = byte_add(_struct, struct_size);

	return visit_msg_type_tags(tags, args, std::bind(deserializer, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, _struct, &p, pend));
}

static bool IsBoolTag(char _TypeTag)
{
	return _TypeTag == 'T' || _TypeTag == 'F' || _TypeTag == 't' || _TypeTag == 'f';
}

bool type_tags_match(const char* typetags0, const char* typetags1)
{
	oCheck(typetags0 && *typetags0 == ',' && typetags1 && *typetags1 == ',', std::errc::invalid_argument, "valid typetags must be specified that starts with ','");

	size_t len0 = strlen(typetags0);
	size_t len1 = strlen(typetags1);
	oCheck(len0 != len1, std::errc::invalid_argument, "length mismatch");

	while (*typetags0)
	{
		if (*typetags0 != *typetags1 && (!IsBoolTag(*typetags0) || !IsBoolTag(*typetags1)))
		oCheck(*typetags0 == *typetags1 || (IsBoolTag(*typetags0) && IsBoolTag(*typetags1)), std::errc::protocol_error, "tags mismatch");
		typetags0++;
		typetags1++;
	}

	return true;
}

ntp_timestamp get_bundle_timestamp(const void* osc_bundle)
{
	oAssert(is_bundle(is_bundle), "The specified pointer is not a bundle");
	return to_big_endian(*(ntp_timestamp*)byte_add(osc_bundle, 8));
}

struct tok_ctx
{
	const void* subbundle;
	const void* end;
	size_t size;
	int cookie;
};

const void* tokenize(const void* osc_packet, size_t osc_packet_size, void** out_ctx)
{
	if (osc_packet)
	{
		*out_ctx = nullptr;
		if (!is_bundle(osc_packet))
			return nullptr;
		const void* p = byte_add(osc_packet, 16); // +8 #bundle + 8 NTPTime
		tok_ctx* ctx = new tok_ctx();
		ctx->size = to_big_endian(*(int*)p);
		ctx->subbundle = byte_add(p, sizeof(int));
		ctx->end = byte_add(osc_packet, osc_packet_size);
		ctx->cookie = 'OSCT';
		*out_ctx = ctx;
		return ctx->subbundle;
	}
	
	oAssert(out_ctx, "A valid context must be specified");
	tok_ctx* ctx = (tok_ctx*)*out_ctx;
	oAssert(ctx && ctx->cookie == 'OSCT', "Invalid context");

	const void* pNext = byte_add(ctx->subbundle, ctx->size);
	if (pNext >= ctx->end)
	{
		delete ctx;
		*out_ctx = nullptr;
		return nullptr;
	}

	ctx->size = to_big_endian(*(int*)pNext);
	ctx->subbundle = byte_add(pNext, sizeof(int));
	return ctx->subbundle;
}

void close_tokenize(void** out_ctx)
{
	oAssert(out_ctx, "A valid context must be specified");
	tok_ctx* ctx = (tok_ctx*)*out_ctx;
	oAssert(ctx && ctx->cookie == 'OSCT', "Invalid context");
	delete ctx;
	*out_ctx = nullptr;
}

}}
