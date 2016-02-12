// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Serial communication interface

#pragma once

namespace ouro {

class serial_port
{
public:
	enum port
	{
		com1,
		com2,
		com3,
		com4,
	};
	
	enum parity
	{
		none,
		odd,
		even,
		mark,
		space,
	};
	
	enum stop_bits
	{
		one,
		one5,
		two,
	};
	
	struct info
	{
		info()
			: com_port(com1)
			, baud(9600)
			, byte_size(8)
			, parity(none)
			, stop_bits(one)
			, read_timeout_ms(200)
			, per_byte_read_timeout_ms(10)
		{}
		
		port com_port;
		int baud;
		int byte_size;
		enum parity parity;
		enum stop_bits stop_bits;
		unsigned int read_timeout_ms;
		unsigned int per_byte_read_timeout_ms;
	};
	
	virtual info get_info() const = 0;
	virtual void send(const void* _pBuffer, size_t _SizeofBuffer) = 0;
	virtual size_t receive(void* _pBuffer, size_t _SizeofBuffer) = 0;

	static std::shared_ptr<serial_port> make(const info& _Info);
};

}
