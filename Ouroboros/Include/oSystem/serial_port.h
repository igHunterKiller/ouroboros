// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Serial communication interface

#pragma once

namespace ouro {

class serial_port
{
public:
	enum class port
	{
		com1,
		com2,
		com3,
		com4,

		count,
	};
	
	enum class parity
	{
		none,
		odd,
		even,
		mark,
		space,

		count,
	};
	
	enum class stop_bits
	{
		one,
		one5,
		two,

		count,
	};
	
	struct info
	{
		info()
			: com_port(port::com1)
			, baud(9600)
			, byte_size(8)
			, parity(parity::none)
			, stop_bits(stop_bits::one)
			, read_timeout_ms(200)
			, per_byte_read_timeout_ms(10)
		{}
		
		port com_port;
		int baud;
		int byte_size;
		parity parity;
		stop_bits stop_bits;
		unsigned int read_timeout_ms;
		unsigned int per_byte_read_timeout_ms;
	};
	
	virtual info get_info() const = 0;
	virtual void send(const void* _pBuffer, size_t _SizeofBuffer) = 0;
	virtual size_t receive(void* _pBuffer, size_t _SizeofBuffer) = 0;

	static std::shared_ptr<serial_port> make(const info& _Info);
};

}
