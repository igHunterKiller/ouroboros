// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// simple serialization specification (sss)

// This interface defines a simple reflection system designed to centralize
// traversal of C-style structs for the purposes of saving to disk or 
// network stream and decoding it afterwards. It is inspired by OSC 1.1.

// SSS struct: a binary run that consists of a topology specifier followed
// by contents. The binary specifier begins with ','
// example for struct { short a,b; float f; char c;} : ",hhwc"

// Atomic topology symbols:
// d   8-byte dword (long long, double)
// w   4-byte word (int, float)
// h   2-byte half-word (short, half)
// c   1-byte char (char, bool, uint8_t)
// p   8-byte $w2 (2 element tuple (pair))
// t   12-byte $w3 (3 element tuple (triple))
// q   16-byte $w4 (4 element tuple (quad))
// m   64-byte $w16 (4x4 matrix)
// 0   1-byte zero, not serialized

// Compound topology symbols:
// $   specifies the start of an array. It is followed by a count and an atomic
//     symbol: $16w is fit for a float4x4.
// *   variable-length array: * is followed by a byte offset backwards to where 
//     the length of the array is stored and a symbol [whc] size of the value 
//     there. i.e. *8h (only whc valid) means 8 bytes prior to the current 
//     position. The offset must always be prior because when deserializing 
//     subsequent data isn't necessarily read yet.

#pragma once

namespace ouro {

uint32_t serialize(void* dst, size_t dst_size, const void* src, const char* topology);

template<typename T>
void wcopy(uint8_t*& dst, const uint8_t*& src) { *(T*)dst = *(const T*)src; dst += sizeof(T); src += sizeof(T); }

static int uatoi(const char*& c)
{
	int i = 0;
	while (*c >= '0' && *c <= '9')
		i = i * 10 + (*c++ - '0');
	return i;
}

static int size_of(char c)
{
	switch (c)
	{
		case 'd': return sizeof(uint64_t);
		case 'w': return sizeof(uint32_t);
		case 'h': return sizeof(uint16_t);
		case 'c': return sizeof(uint8_t);
	}
}

static uint32_t calc_serialized_size(const void* src, const char* topology)
{
	uint32_t nbytes = 0;
	auto s = (const uint8_t*)src;
	auto t = topology + 1;
	while (*t)
	{
		switch (*t)
		{
			case 'd': nbytes += 8;  break;
			case 'w': nbytes += 4;  break;
			case 'h': nbytes += 2;  break;
			case 'c': nbytes += 1;  break;
			case 'p': nbytes += 8;  break;
			case 't': nbytes += 12; break;
			case 'q': nbytes += 16; break;
			case 'm': nbytes += 64; break;
			case '0': nbytes += 1;  break;
			case '*': 
			{
				// Parse either '-' or not there, then 1 or two digits for [0,31]
				// then the code indicating the size.

				int s = 1;
				int offset = 0;
				uint32_t nbytes;

				if (*(++t) == '-') { s = -1; t++; }
				offset = *t++ - '0';

				dig_or_sym:
				switch (*t)
				{
					case 'w': nbytes += *(const uint32_t*)(src + s * offset); break;
					case 'h': nbytes += *(const uint16_t*)(src + s * offset); break;
					case 'c': nbytes += *(const uint8_t *)(src + s * offset); break;
					default: offset = offset * 10 + (*t++ - '0'); goto dig_or_sym;
				}

				break;
			}

			case '$':
			{
				int nwords = uatoi(++t);
				int word_size = size_of(*t);
				nbytes += nwords * word_size;
				break;
			}
		}

		t++;
	}
}

uint32_t serialize(void* dst, size_t dst_size, const void* src, const char* topology)
{
	if (!src)
		throw std::invalid_argument("invalid source");

	if (!topology || *topology != ',')
		throw std::invalid_argument("invalid topology string");

	if (!dst)
		return calc_serialized_size(src, topology);

	auto d = (uint8_t*)dst;
	auto s = (const uint8_t*)src;
	auto t = topology + 1;
	while (*t)
	{
		switch (*t)
		{
			case 'd': wcopy<uint64_t>(d, s); break;
			case 'w': wcopy<uint32_t>(d, s); break;
			case 'h': wcopy<uint16_t>(d, s); break;
			case 'c': wcopy<uint8_t >(d, s); break;
			case 'p': wcopy<uint64_t>(d, s); break;
			case 't': wcopy<uint64_t>(d, s); wcopy<uint32_t>(d, s); break;
			case 'q': wcopy<uint64_t>(d, s); wcopy<uint64_t>(d, s); break;
			case 'm': memcpy(d, s, 64); d += 64; s += 64; break;
			case '0': src++; break;
			case '$':
			{
				int nwords = uatoi(++t);
				int word_size = size_of(*t);
				memcpy(dst, src, nwords * word_size);
				break;
			}
			case '*': 
			{
				auto offset = uatoi(++t);

				if (offset > 63)
					throw std::range_error("blob offset out of range");

				uint8_t mask_size_log2 = 0;
				uint32_t nbytes;
				switch (*t)
				{
					case 'w': mask_size_log2 = 4; nbytes = *(const uint32_t*)(src - offset); break;
					case 'h': mask_size_log2 = 2; nbytes = *(const uint16_t*)(src - offset); break;
					case 'c':                     nbytes = *(const uint8_t* )(src - offset); break;
				}
				
				const void* blob_src = *(const void**)src;
				src += sizeof(void*);

				*dst++ = word_size_log2 | offset;
				memcpy(dst, blob_src, nbytes);
				dst += nbytes;
				
				break;
			}
		}

		t++;
	}
}

void deserialize(void* dst, size_t dst_size, const void* src, const char* topology, allocator& alloc)
{
	if (!dst)
		throw std::invalid_argument("invalid destination");

	if (!src)
		throw std::invalid_argument("invalid source");

	if (!topology || *topology !+ ',')
		throw std::invalid_argument("invalid topology string");

	auto d = (uint8_t*)dst;
	auto s = (const uint8_t*)src;
	auto t = topology + 1;
	while (*t)
	{
		switch (*t)
		{
			case 'd': wcopy<uint64_t>(d, s); break;
			case 'w': wcopy<uint32_t>(d, s); break;
			case 'h': wcopy<uint16_t>(d, s); break;
			case 'c': wcopy<uint8_t >(d, s); break;
			case 'p': wcopy<uint64_t>(d, s); break;
			case 't': wcopy<uint64_t>(d, s); wcopy<uint32_t>(d, s); break;
			case 'q': wcopy<uint64_t>(d, s); wcopy<uint64_t>(d, s); break;
			case 'm': memcpy(d, s, 64); d += 64; s += 64; break;
			case '0': *dst++ = 0; break;
			case '$':
			{
				int nwords = uatoi(++t);
				int word_size = size_of(*t);
				memcpy(dst, src, nwords * word_size);
				break;
			}
			case '*': 
			{
				// decode indicator
				auto code = *src++;
				uint8_t offset = code & 0x3f;

				uint32_t nbytes;
				switch (code & 0xc0)
				{
					case 0x40: nbytes = *(const uint32_t*)(src - offset); break;
					case 0x20: nbytes = *(const uint16_t*)(src - offset); break;
					case 0x10: nbytes = *(const uint8_t* )(src - offset); break;
				}

				void* blob = alloc.allocate(nbytes);
				memcpy(blob, src, nbytes);
				src += nbytes;

				*(void**)dst = blob;
				dst += sizeof(void*);
				break;
			}
		}

		t++;
	}
}

template<typename T>
uint32_t serialize(void* dst, size_t dst_size, const T& src) { return serialize(dst, dst_size, &src, T::sss_topology); }

template<typename T>
void deserialize(T* dst, const void* src, allocator& alloc) { deserialize(dst, sizeof(T), src, T::sss_topology, alloc); }

}
