// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/finally.h>
#include <oSystem/serial_port.h>

#include <oSystem/windows/win_error.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ouro {

template<> const char* as_string(const serial_port::port& port)
{
	static const char* s_names[] = 
	{
		"COM1",
		"COM2",
		"COM3",
		"COM4",
	};
	return as_string(port, s_names);
}

static unsigned char get_stop_bits(const serial_port::stop_bits& stop_bits)
{
	switch(stop_bits)
	{
		case serial_port::stop_bits::one:  return ONESTOPBIT;
		case serial_port::stop_bits::one5: return ONE5STOPBITS;
		case serial_port::stop_bits::two:  return TWOSTOPBITS;
		default: break;
	}
	return ONESTOPBIT;
}

static unsigned char get_parity(const serial_port::parity& parity)
{
	switch (parity)
	{
		case serial_port::parity::even:  return EVENPARITY;
		case serial_port::parity::odd:   return ODDPARITY;
		case serial_port::parity::none:  return NOPARITY;
		case serial_port::parity::mark:  return MARKPARITY;
		case serial_port::parity::space: return SPACEPARITY;
		default: break;
	}
	return NOPARITY;
}

class serial_port_impl : public serial_port
{
public:
	serial_port_impl(const info& _Info);
	~serial_port_impl();
	info get_info() const override;
	void send(const void* _pBuffer, size_t _SizeofBuffer) override;
	size_t receive(void* _pBuffer, size_t _SizeofBuffer) override;

private:
	HANDLE hFile;
	info Info;
};

serial_port_impl::serial_port_impl(const info& _Info)
	: Info(_Info)
	, hFile(INVALID_HANDLE_VALUE)
{
	HANDLE hNewFile = CreateFile(as_string(_Info.com_port), GENERIC_READ|GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	oCheck(hNewFile != INVALID_HANDLE_VALUE, std::errc::no_such_device, "%s does not exist", as_string(_Info.com_port));
	oFinally { if (hNewFile != INVALID_HANDLE_VALUE) CloseHandle(hNewFile); };

	DCB dcb = {0};
	dcb.DCBlength = sizeof(DCB);
	oVB(GetCommState(hNewFile, &dcb));

	dcb.BaudRate = Info.baud;
	dcb.ByteSize = (BYTE)Info.byte_size;
	dcb.StopBits = get_stop_bits(Info.stop_bits);
	dcb.Parity = get_parity(Info.parity);

	oVB(SetCommState(hNewFile, &dcb));

	COMMTIMEOUTS cto;
	oVB(GetCommTimeouts(hNewFile, &cto));

	cto.ReadIntervalTimeout = Info.read_timeout_ms;
	cto.ReadTotalTimeoutConstant = Info.read_timeout_ms;
	cto.ReadTotalTimeoutMultiplier = Info.per_byte_read_timeout_ms;

	oVB(SetCommTimeouts(hNewFile, &cto));

	hFile = hNewFile;
	hNewFile = INVALID_HANDLE_VALUE; // prevent finally from killing the successful file
}

std::shared_ptr<serial_port> serial_port::make(const serial_port_impl::info& _Info)
{
	return std::make_shared<serial_port_impl>(_Info);
}

serial_port_impl::~serial_port_impl()
{
	if (hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
}

serial_port_impl::info serial_port_impl::get_info() const
{
	return Info;
}

void serial_port_impl::send(const void* _pBuffer, size_t _SizeofBuffer)
{
	DWORD written = 0;
	oVB(WriteFile(hFile, _pBuffer, static_cast<DWORD>(_SizeofBuffer), &written, nullptr));
	if (written != static_cast<DWORD>(_SizeofBuffer))
	{
		sstring b1, b2;
		format_bytes(b1, _SizeofBuffer, 2);
		format_bytes(b2, written, 2);
		oThrow(std::errc::io_error, "requested send of %s, but sent %d", b1.c_str(), b2.c_str());
	}
}

size_t serial_port_impl::receive(void* _pBuffer, size_t _SizeofBuffer)
{
	DWORD read = 0;
	oVB(ReadFile(hFile, _pBuffer, static_cast<DWORD>(_SizeofBuffer), &read, nullptr));
	return read;
}

}
