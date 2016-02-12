// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Object encapsulating an ip4 internet address with port

#pragma once

namespace ouro {

class ip4
{
public:
	// remember endianness. The 32-bit version of the address is expected
	// in big-endian order, meaning the leftmost byte represents the 
	// leftmost address octet.
	ip4(unsigned char x, unsigned char y, unsigned char z, unsigned char w, unsigned short port = 0) : port_(port) { octet_[0] = x; octet_[1] = y; octet_[2] = z; octet_[3] = w; }
	ip4(unsigned int address, unsigned short port = 0) : address_(address), port_(port) {}
		
	inline void address(unsigned int a) { address_ = a; }
	inline unsigned int address() const { return address_; }

	inline unsigned char octet(size_t index) const { return octet_[index]; }
	inline void octet(size_t index, unsigned char x) { octet_[index] = x; }

	inline void port(unsigned short p) { port_ = p; }
	inline unsigned short port() const { return port_; }

	inline bool local() const { return address_ == localhost_address; }

private:
	static const unsigned int localhost_address = 0x0100007f; // 127.0.0.1
	union { unsigned int address_; unsigned char octet_[4]; };
	unsigned short port_;
};

}