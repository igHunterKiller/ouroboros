// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

#include <oCore/countof.h>
#include <oCore/finally.h>
#include <oCore/guid.h>
#include <oCore/macros.h>
#include <oCore/stringf.h>
#include <oSystem/windows/win_winsock.h>

namespace ouro { namespace windows { namespace winsock {

struct WSA_ERR
{
	HRESULT hresult;
	const char* errstr;
	std::errc::errc err;
	const char* errdesc;
};

#ifndef ESOCKTNOSUPPORT
	#define ESOCKTNOSUPPORT -37
#endif
#ifndef ESHUTDOWN
	#define ESHUTDOWN -36
#endif

#define WSAERR(x) x, #x
#define WSAERRNO(x) WSA##x, "WSA" #x, std::errc::errc(x)
#define WSAERRNOX(x) WSA##x, "WSA" #x, std::errc::protocol_error
static const WSA_ERR sWSAErrors[] =
{
	{ WSAERR(WSANOTINITIALISED), std::errc::state_not_recoverable , "Either the application has not called WSAStartup or WSAStartup failed. The application may be accessing a socket that the current active task does not own (that is, trying to share a socket between tasks), or WSACleanup has been called too many times." },
	{ WSAERRNOX(ENETDOWN), 0 },
	{ WSAERR(WSAVERNOTSUPPORTED), std::errc::operation_not_supported, 0 },
	{ WSAERRNOX(EAFNOSUPPORT), 0 },
	{ WSAERRNOX(EINPROGRESS), 0 },
	{ WSAERRNO(EMFILE), 0 },
	{ WSAERRNO(EINVAL), 0 },
	{ WSAERRNOX(ENOBUFS), 0 },
	{ WSAERRNOX(EPROTONOSUPPORT), 0 },
	{ WSAERRNOX(EPROTOTYPE), 0 },
	{ WSAERRNO(ESOCKTNOSUPPORT), 0 },
	{ WSAERR(WSASYSNOTREADY), std::errc::protocol_not_supported, 0 },
	{ WSAERR(WSAEPROCLIM), std::errc::too_many_links, 0 },
	{ WSAERRNO(EFAULT), 0 },
	{ WSAERRNOX(ENOTSOCK), 0 },
	{ WSAERRNOX(ENETRESET), 0 },
	{ WSAERRNOX(ENOTCONN), 0 },
	{ WSAERRNOX(EADDRNOTAVAIL), 0 },
	{ WSAERRNOX(ECONNREFUSED), 0 },
	{ WSAERRNOX(ECONNABORTED), 0 },
	{ WSAERRNOX(ECONNRESET), 0 },
	{ WSAERRNOX(ENETUNREACH), 0 },
	{ WSAERRNOX(EHOSTUNREACH), 0 },
	{ WSAERRNOX(ETIMEDOUT), 0 },
	{ WSAERRNOX(EWOULDBLOCK), 0 },
	{ WSAERRNO(EACCES), 0 },
	{ WSAERRNOX(EADDRINUSE), 0 },
	{ WSAERRNOX(EOPNOTSUPP), 0 },
	{ WSAERRNO(ESHUTDOWN), 0 },
	{ WSAERRNOX(EMSGSIZE), 0 },
};

const char* as_string_WSAerr(int _WSAError)
{
	for (const auto& err : sWSAErrors)
		if (_WSAError == err.hresult)
			return err.errstr;
	return "?";
}

const char* get_desc(int _WSAError)
{
	for (const auto& err : sWSAErrors)
		if (_WSAError == err.hresult && err.errdesc)
			return err.errdesc;
	
	return "See http://msdn.microsoft.com/en-us/library/ms740668(v=vs.85).aspx for more information.";
}

std::errc::errc get_errc(int _WSAError)
{
	for (const auto& err : sWSAErrors)
		if (_WSAError == err.hresult)
			return err.err;
	return std::errc::state_not_recoverable;
}

LPFN_CONNECTEX getfn_ConnectEx(SOCKET _hSocket)
{
	DWORD dwBytesReturned = 0;
	guid_t g = WSAID_CONNECTEX;
	LPFN_CONNECTEX pConnectEx = nullptr;
	oWSAVB(WSAIoctl(_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &g, sizeof(g), &pConnectEx, sizeof(LPFN_CONNECTEX), &dwBytesReturned, 0, 0));
	return pConnectEx;
}

LPFN_DISCONNECTEX getfn_DisconnectEx(SOCKET _hSocket)
{
	DWORD dwBytesReturned = 0;
	guid_t g = WSAID_DISCONNECTEX;
	LPFN_DISCONNECTEX pDisconnectEx = nullptr;
	oWSAVB(WSAIoctl(_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &g, sizeof(g), &pDisconnectEx, sizeof(LPFN_DISCONNECTEX), &dwBytesReturned, 0, 0));
	return pDisconnectEx;
}

LPFN_GETACCEPTEXSOCKADDRS getfn_GetAcceptExSockaddrs(SOCKET _hSocket)
{
	DWORD dwBytesReturned = 0;
	guid_t g = WSAID_GETACCEPTEXSOCKADDRS;
	LPFN_GETACCEPTEXSOCKADDRS pGetAcceptExSockaddrs = nullptr;
	oWSAVB(WSAIoctl(_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &g, sizeof(g), &pGetAcceptExSockaddrs, sizeof(LPFN_GETACCEPTEXSOCKADDRS), &dwBytesReturned, 0, 0));
	return pGetAcceptExSockaddrs;
}

LPFN_TRANSMITPACKETS getfn_TransmitPackets(SOCKET _hSocket)
{
	DWORD dwBytesReturned = 0;
	guid_t g = WSAID_TRANSMITPACKETS;
	LPFN_TRANSMITPACKETS pTransmitPackets = nullptr;
	oWSAVB(WSAIoctl(_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &g, sizeof(g), &pTransmitPackets, sizeof(LPFN_TRANSMITPACKETS), &dwBytesReturned, 0, 0));
	return pTransmitPackets;
}

LPFN_WSARECVMSG getfn_WSARecvMsg(SOCKET _hSocket)
{
	DWORD dwBytesReturned = 0;
	guid_t g = WSAID_WSARECVMSG;
	LPFN_WSARECVMSG pWSARecvMsg = nullptr;
	oWSAVB(WSAIoctl(_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &g, sizeof(g), &pWSARecvMsg, sizeof(LPFN_WSARECVMSG), &dwBytesReturned, 0, 0));
	return pWSARecvMsg;
}

LPFN_ACCEPTEX getfn_AcceptEx(SOCKET _hSocket)
{
	DWORD dwBytesReturned = 0;
	guid_t g = WSAID_ACCEPTEX;
	LPFN_ACCEPTEX pAcceptEx = nullptr;
	oWSAVB(WSAIoctl(_hSocket, SIO_GET_EXTENSION_FUNCTION_POINTER, &g, sizeof(g), &pAcceptEx, sizeof(LPFN_ACCEPTEX), &dwBytesReturned, 0, 0));
	return pAcceptEx;
}

sockaddr_in make_addr(const char* _Hostname)
{
	char host[256];
	strlcpy(host, _Hostname);
	unsigned short port = 0;
	char* strPort = strstr(host, ":");
	if (strPort)
	{
		port = static_cast<unsigned short>(atoi(strPort+1));
		*strPort = 0;
	}

	sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	hostent* pHost = gethostbyname(host);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = inet_addr(inet_ntoa(*(in_addr*)*pHost->h_addr_list));
	return sa;
}

char* addr_to_hostname(char* _StrDestination, size_t _SizeofStrDestination, const sockaddr_in& _SockAddr)
{
	const unsigned long addr = ntohl(_SockAddr.sin_addr.s_addr);
	const unsigned short port = ntohs(_SockAddr.sin_port);
	if (-1 == snprintf(_StrDestination, _SizeofStrDestination
		, "%u.%u.%u.%u:%u"
		, (addr&0xFF000000)>>24, (addr&0xFF0000)>>16, (addr&0xFF00)>>8, addr&0xFF
		, port))
		throw std::system_error(std::errc::no_buffer_space, std::system_category());
	return _StrDestination;
}

static void get_name_base(char* _StrOutHostname, size_t _SizeofStrOutHostname, char* _StrOutIPAddress, size_t _SizeofStrOutIPAddress, char* _StrOutPort, size_t _SizeofStrOutPort, SOCKET _hSocket, const std::function<int(SOCKET _hSocket, sockaddr* _Name, int* _pNameLen)>& _GetSockAddr)
{
	sockaddr_in saddr;
	int size = sizeof(saddr);
	oWSAVB(_GetSockAddr(_hSocket, (sockaddr*)&saddr, &size));

	// Allow for the user to specify null for the parts they don't want
	char localHostname[_MAX_PATH];
	char localService[16];

	char* pHostname = _StrOutHostname ? _StrOutHostname : localHostname;
	size_t sizeofHostname = _StrOutHostname ? _SizeofStrOutHostname : countof(localHostname);
	
	char* pService = _StrOutPort ? _StrOutPort : localService;
	size_t sizeofService = _StrOutPort ? _SizeofStrOutPort : countof(localService);

	getnameinfo((sockaddr*)&saddr, size, pHostname, static_cast<DWORD>(sizeofHostname), pService, static_cast<DWORD>(sizeofService), 0);

	if (_StrOutIPAddress)
	{
		const char* ip = inet_ntoa(saddr.sin_addr);
		if (strlcpy(_StrOutIPAddress, ip, _SizeofStrOutIPAddress) >= _SizeofStrOutIPAddress)
			throw std::system_error(std::errc::no_buffer_space, std::system_category());
	}
}

void get_hostname(char* _StrOutHostname, size_t _SizeofStrOutHostname, char* _StrOutIPAddress, size_t _SizeofStrOutIPAddress, char* _StrOutPort, size_t _SizeofStrOutPort, SOCKET _hSocket)
{
	get_name_base(_StrOutHostname, _SizeofStrOutHostname, _StrOutIPAddress, _SizeofStrOutIPAddress, _StrOutPort, _SizeofStrOutPort, _hSocket, getsockname);
}

void get_peername(char* _StrOutHostname, size_t _SizeofStrOutHostname, char* _StrOutIPAddress, size_t _SizeofStrOutIPAddress, char* _StrOutPort, size_t _SizeofStrOutPort, SOCKET _hSocket)
{
	get_name_base(_StrOutHostname, _SizeofStrOutHostname, _StrOutIPAddress, _SizeofStrOutIPAddress, _StrOutPort, _SizeofStrOutPort, _hSocket, getpeername);
}

unsigned short get_port(SOCKET _hSocket)
{
	sockaddr_in sa;
	int size = sizeof(sa);
	oWSAVB(getsockname(_hSocket, (sockaddr*)&sa, &size));
	return ntohs(sa.sin_port);
}

// Not sure in what form we're sending data, so the flags may not be the ones
// to test.
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms740094(v=vs.85).aspx

bool connected(SOCKET _hSocket)
{
	WSAPOLLFD p;
	p.fd = _hSocket;
	p.events = POLLRDNORM|POLLWRNORM;
	p.revents = 0;
	oWSAVBE(WSAPoll(&p, 1, 0), WSAENETDOWN);
	return ((p.revents & POLLHUP) != POLLHUP);
}

bool connected2(SOCKET _hSocket)
{
	WSAPOLLFD p;
	p.fd = _hSocket;
	p.events = POLLRDNORM|POLLWRNORM;
	p.revents = 0;
	oWSAVBE(WSAPoll(&p, 1, 0), WSAENETDOWN);
	return ((p.revents & POLLRDNORM) == POLLRDNORM) || ((p.revents & POLLWRNORM) == POLLWRNORM);
}

void enumerate_addresses(const std::function<void(const sockaddr_in& _Addr)> _Enumerator)
{
	// Enumerates the addresses of all attached interfaces.
	// From http://support.microsoft.com/kb/129315
	if (_Enumerator)
	{
		char HostName[64];
		gethostname(HostName, countof(HostName));
		HOSTENT* pHostEntry = gethostbyname(HostName);

		int Adapter = 0;
		while (pHostEntry->h_addr_list[Adapter])
		{
			sockaddr_in sAddr;
			memcpy(&sAddr.sin_addr.s_addr, pHostEntry->h_addr_list[Adapter], pHostEntry->h_length);
			_Enumerator(sAddr);
			Adapter++;
		}
	}
}

bool wait_multiple(WSAEVENT* _pHandles, size_t _NumberOfHandles, bool _WaitAll, unsigned int _TimeoutMS)
{
	return WSA_WAIT_TIMEOUT != WSAWaitForMultipleEvents(static_cast<DWORD>(_NumberOfHandles), _pHandles, _WaitAll, _TimeoutMS == ~0u ? WSA_INFINITE : _TimeoutMS, FALSE);
}

// WSA_FLAG_NO_HANDLE_INHERIT
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms742212(v=vs.85).aspx
// Create a socket that is non-inheritable.
// A socket handle created by the WSASocket or the socket function is inheritable by default. When this flag is set, the socket handle is non-inheritable.
// The GetHandleInformation function can be used to determine if a socket handle was created with the WSA_FLAG_NO_HANDLE_INHERIT flag set. The GetHandleInformation function will return that the HANDLE_FLAG_INHERIT value is set.
// This flag is supported on Windows 7 with SP1, Windows Server 2008 R2 with SP1, and later
#ifndef WSA_FLAG_NO_HANDLE_INHERIT
	#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

template<typename SOCKADDR_T>
SOCKET makeT(const SOCKADDR_T& _Addr, int _Options, unsigned int _TimeoutMS, unsigned int _MaxNumConnections)
{
	if (_MaxNumConnections < 0)
		_MaxNumConnections = SOMAXCONN;

	WSAEVENT hConnectEvent = nullptr;
	bool CloseSocketOnExit = false;

	const bool kIsReliable = !!(_Options & reliable);
	SOCKET hSocket = WSASocket(AF_INET, kIsReliable ? SOCK_STREAM : SOCK_DGRAM, kIsReliable ? IPPROTO_TCP : IPPROTO_UDP, 0, 0, WSA_FLAG_OVERLAPPED | WSA_FLAG_NO_HANDLE_INHERIT);

	oFinally 
	{
		if (CloseSocketOnExit)
			closesocket(hSocket);
		if (hConnectEvent)
			WSACloseEvent(hConnectEvent);
	};

	if (hSocket == INVALID_SOCKET)
		oWSATHROWLAST();

	CloseSocketOnExit = true; // in case of error
	
	u_long enabled = !(_Options & blocking);

	oWSAVB(ioctlsocket(hSocket, FIONBIO, &enabled));

	if (_Options & reuse_address) 
		oWSAVB(setsockopt(hSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enabled, sizeof(enabled)));

	if (_Options & exclusive_address) 
		oWSAVB(setsockopt(hSocket, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&enabled, sizeof(enabled)));

	if (!kIsReliable && (_Options & allow_broadcast))
		oWSAVB(setsockopt(hSocket, SOL_SOCKET, SO_BROADCAST, (const char*)&enabled, sizeof(enabled)));
	
	if (kIsReliable)
	{
		if (_MaxNumConnections)
		{
			oWSAVB(bind(hSocket, (const sockaddr*)&_Addr, sizeof(_Addr)));
			oWSAVB(listen(hSocket, _MaxNumConnections));
		}

		else
		{
			if (_TimeoutMS > 0)
			{
				hConnectEvent = WSACreateEvent();
				oWSAVB(WSAEventSelect(hSocket, hConnectEvent, FD_CONNECT));
			}

			oWSAVBE(connect(hSocket, (const sockaddr*)&_Addr, sizeof(_Addr)), WSAEWOULDBLOCK);

			if (hConnectEvent)
			{
				if (!wait(hConnectEvent, _TimeoutMS))
					oWSAVB(false);
				else
				{
					if (!connected2(hSocket))
						oWSATHROW0(WSAEHOSTUNREACH);
					oWSAVB(WSAEventSelect(hSocket, nullptr, 0));
					unsigned long cmd = 0;
					oWSAVB(ioctlsocket(hSocket, FIONBIO, &cmd));
				}
			}
		}
	}
	else
		oWSAVB(bind(hSocket, (const sockaddr*)&_Addr, sizeof(_Addr)));

	CloseSocketOnExit = false; // no error, don't clean up the new socket
	return hSocket;
}

SOCKET make(const sockaddr_in& _Addr, int _Options, unsigned int _TimeoutMS /*= 0*/, unsigned int _MaxNumConnections /*= 0*/)
{
	return makeT(_Addr, _Options, _TimeoutMS, _MaxNumConnections);
}

SOCKET make(const sockaddr_in6& _Addr, int _Options, unsigned int _TimeoutMS /*= 0*/, unsigned int _MaxNumConnections /*= 0*/)
{
	return makeT(_Addr, _Options, _TimeoutMS, _MaxNumConnections);
}

SOCKET make_async_accept()
{
	SOCKET hSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 0, 0, WSA_FLAG_OVERLAPPED);
	if (hSocket == INVALID_SOCKET)
		oWSATHROWLAST();
	return hSocket;
}

void finish_async_accept(SOCKET _hListenSocket, SOCKET _hAcceptSocket)
{
	oWSAVB(setsockopt(_hAcceptSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)&_hListenSocket, sizeof(_hListenSocket)));
}

async_result recycle_async_accept(SOCKET _hListenSocket, SOCKET _hAcceptSocket, WSAOVERLAPPED* _pOverlapped)
{
	LPFN_DISCONNECTEX pDisconnectEx = getfn_DisconnectEx(_hListenSocket);
	if (!pDisconnectEx(_hAcceptSocket, _pOverlapped, TF_REUSE_SOCKET, 0))
		return WSAGetLastError() == ERROR_IO_PENDING ? scheduled : failed;
	return completed;
}

void close(SOCKET _hSocket)
{
	// http://msdn.microsoft.com/en-us/library/ms738547(v=vs.85).aspx
	// Specifically read the community comment that says the main article doesn't 
	// work. Also ignore WSAENOTCONN since we're closing this socket anyway - it 
	// means the other side is detached already.

	if (_hSocket && _hSocket != INVALID_SOCKET)
	{
		int err = shutdown(_hSocket, SD_BOTH);
		if (err == SOCKET_ERROR)
		{
			err = shutdown(_hSocket, SD_SEND);
			if (err == SOCKET_ERROR)
				err = shutdown(_hSocket, SD_RECEIVE);
		}
		
		if (err == SOCKET_ERROR && WSAGetLastError() != WSAENOTCONN && WSAGetLastError() != WSAEINVAL)
			oWSATHROWLAST();
		LPFN_DISCONNECTEX pDisconnectEx = getfn_DisconnectEx(_hSocket);
		if (!pDisconnectEx(_hSocket, 0, 0, 0) && WSAGetLastError() != WSAENOTCONN)
			oWSATHROWLAST();
		oWSAVBE(closesocket(_hSocket), WSAENOTCONN);
	}
}

void set_keepalive(SOCKET _hSocket, unsigned int _TimeoutMS, unsigned int _IntervalMS)
{
	if (_hSocket)
	{
		tcp_keepalive KeepAlive;
		KeepAlive.onoff = 1;
		KeepAlive.keepalivetime = _TimeoutMS;
		KeepAlive.keepaliveinterval = _IntervalMS;
		DWORD dwBytesReturned = 0;
		oWSAVB(WSAIoctl(_hSocket, SIO_KEEPALIVE_VALS, &KeepAlive.onoff, sizeof(KeepAlive), NULL, 0, &dwBytesReturned, 0, 0));
	}
}

#define FD_TRACE(TracePrefix, TraceName, FDEvent) oTRACE("%s%s%s: %s (%s)", oSAFESTR(TracePrefix), _TracePrefix ? " " : "", oSAFESTR(TraceName), #FDEvent, get_desc(_pNetworkEvents->iErrorCode[FDEvent##_BIT]))
//#define FD_TRACE(TracePrefix, TraceName, FDEvent)

// Assumes WSANETWORKEVENTS ne; int err;
#define FD_CHECK(FDEvent) \
	if (_pNetworkEvents->lNetworkEvents & FDEvent) \
	{	FD_TRACE(_TracePrefix, _TraceName, ##FDEvent); \
		err = _pNetworkEvents->iErrorCode[FDEvent##_BIT]; \
	}

// Checks all values from a network event and return an error based on what happened.
static void wsatrace(const char* _TracePrefix, const char* _TraceName, const WSANETWORKEVENTS* _pNetworkEvents)
{
	int err = 0;

	if (!_pNetworkEvents->lNetworkEvents)
	{
		// http://www.mombu.com/microsoft/alt-winsock-programming/t-wsaenumnetworkevents-returns-no-event-how-is-this-possible-1965867.html
		// (spurious wakeup)
		throw std::system_error(std::errc::protocol_error, std::system_category(), stringf("%s%s%s: WSAEVENT, but no lNetworkEvent: \"spurious wakeup\". You should ignore the event as if it never happened by testing for _pNetworkEvents->lNetworkEvents == 0 in calling code."
			, oSAFESTR(_TracePrefix), _TracePrefix ? " " : "", oSAFESTR(_TraceName)));
	}
	
	FD_CHECK(FD_READ); FD_CHECK(FD_WRITE); FD_CHECK(FD_OOB); FD_CHECK(FD_CONNECT); 
	FD_CHECK(FD_ACCEPT); FD_CHECK(FD_CLOSE); FD_CHECK(FD_QOS); FD_CHECK(FD_GROUP_QOS); 
	FD_CHECK(FD_ROUTING_INTERFACE_CHANGE); FD_CHECK(FD_ADDRESS_LIST_CHANGE);
}

// If the socket was created using wait (WSAEventSelect()) this function can 
// be used to wait on that event and receive any events breaking the wait. This 
// function handles spurious wakeups so if using WSANETWORKEVENTS structs, 
// always use this wrapper.
static bool wait(SOCKET _hSocket, WSAEVENT _hEvent, WSANETWORKEVENTS* _pNetEvents, unsigned int _TimeoutMS)
{
	bool eventFired = true;
	_pNetEvents->lNetworkEvents = 0;
	unsigned int Start = timer::now_msi();
	while (!_pNetEvents->lNetworkEvents && eventFired)
	{
		eventFired = wait(_hEvent, _TimeoutMS);
		if (eventFired)
		{
			oWSAVB(WSAEnumNetworkEvents(_hSocket, _hEvent, _pNetEvents));
			if (_pNetEvents->lNetworkEvents)
				break;

			unsigned int Now = timer::now_msi();
			unsigned int Diff = Now - Start;
			if (Diff < _TimeoutMS)
				_TimeoutMS -= Diff;
		}
	}

	return eventFired;
}

void send(SOCKET _hSocket, const void* _pSource, size_t _SizeofSource, const sockaddr_in* _pDestination)
{
	oASSERT(_SizeofSource <= INT_MAX, "Underlying implementation uses 32-bit signed int for buffer size.");
	int bytesSent = 0;
	if (_pDestination)
		bytesSent = sendto(_hSocket, (const char*)_pSource, static_cast<int>(_SizeofSource), 0, (const sockaddr*)_pDestination, sizeof(sockaddr_in));
	else
		bytesSent = ::send(_hSocket, (const char*)_pSource, static_cast<int>(_SizeofSource), 0);

	if (bytesSent == SOCKET_ERROR || size_t(bytesSent) != _SizeofSource)
		oWSATHROWLAST();
}

size_t receive(SOCKET _hSocket, WSAEVENT _hEvent, void* _pDestination, size_t _SizeofDestination, unsigned int _TimeoutMS, std::atomic<int>* _pInOutCanReceive, sockaddr_in* _pSource)
{
	if (!_pInOutCanReceive)
		throw std::invalid_argument("_pInOutCanReceive must be specified");

	if (_SizeofDestination >= INT_MAX)
		throw std::invalid_argument("Underlying implementation uses 32-bit signed int for buffer size.");

	if (!_pDestination)
		throw std::invalid_argument("Must specify a destination buffer");

	int err = 0;
	WSANETWORKEVENTS ne;
	memset(&ne, 0, sizeof(ne));
	if (*_pInOutCanReceive)
	{
		err = WSAETIMEDOUT;
		bool eventFired = wait(_hSocket, _hEvent, &ne, _TimeoutMS);
		if (eventFired)
		{
			wsatrace("winsock::receive", 0, &ne);
			err = 0;
		}
	}

	int bytesReceived = 0;
	if (!err && (ne.lNetworkEvents & FD_READ))
	{
		if (_pSource)
		{
			int size = sizeof(sockaddr_in);
			bytesReceived = recvfrom(_hSocket, (char*)_pDestination, static_cast<int>(_SizeofDestination), 0, (sockaddr*)_pSource, &size);
		}

		else
			bytesReceived = recv(_hSocket, (char*)_pDestination, static_cast<int>(_SizeofDestination), 0);

		if (bytesReceived == SOCKET_ERROR)
		{
			err = get_errc(WSAGetLastError());
			bytesReceived = 0;
		}

		else if (!bytesReceived)
		{
			*_pInOutCanReceive = false;
			err = ESHUTDOWN;
		}

		else
			err = 0;
	}

	else if ((ne.lNetworkEvents & FD_CLOSE) || ((ne.lNetworkEvents & FD_CONNECT) && err))
	{
		*_pInOutCanReceive = false;
		oWSATHROW0(ne.iErrorCode[FD_CLOSE_BIT]);
	}

	return bytesReceived;
}

size_t receive_nonblocking(SOCKET _hSocket, WSAEVENT _hEvent, void* _pDestination, size_t _SizeofDestination, sockaddr_in* _pSource)
{
	if (_SizeofDestination >= INT_MAX)
		throw std::invalid_argument("Underlying implementation uses 32-bit signed int for buffer size.");

	if (!_pDestination)
		throw std::invalid_argument("Must specify a destination buffer");

	timeval waitTime;
	waitTime.tv_sec = 0;
	waitTime.tv_usec = 0;

	fd_set set;
	FD_ZERO(&set);
	FD_SET(_hSocket, &set);

	if (!select(1, &set, nullptr, nullptr, &waitTime))
		return 0;

	int err = 0;
	int bytesReceived = 0;
	if (_pSource)
	{
		int size = sizeof(sockaddr_in);
		bytesReceived = recvfrom(_hSocket, (char*)_pDestination, static_cast<int>(_SizeofDestination), 0, (sockaddr*)_pSource, &size);
	}

	else
		bytesReceived = recv(_hSocket, (char*)_pDestination, static_cast<int>(_SizeofDestination), 0);

	if (bytesReceived == SOCKET_ERROR)
		oWSATHROWLAST();

	else if (!bytesReceived)
		throw std::system_error(std::errc::connection_reset, std::system_category()); // ESHUTDOWN

	//else if ((ne.lNetworkEvents & FD_CLOSE) || ((ne.lNetworkEvents & FD_CONNECT) && err))
	//{
	//	atomic_exchange(_pInOutCanReceive, false);
	//	oWSATHROW0(ne.iErrorCode[FD_CLOSE_BIT]);
	//}

	return bytesReceived;
}

size_t recvfrom_blocking(SOCKET _hSocket, void* _pDestination, size_t _SizeofDestination, unsigned int _TimeoutMS, const SOCKADDR_IN& _RecvAddr, unsigned int _recvfromFlags)
{
	// ouro::infinite doesn't seem to work with setsockopt so use a sane max
	static const uint32_t kSaneMaxTimoutMS = 60000;
	if (~0u == _TimeoutMS)
		_TimeoutMS = kSaneMaxTimoutMS;

	oWSAVB(setsockopt(_hSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&_TimeoutMS, sizeof(unsigned int)));

	int AddrSize = sizeof(_RecvAddr);
	size_t TotalReceived = recvfrom(_hSocket, (char*)_pDestination, (int)_SizeofDestination, _recvfromFlags, (sockaddr*)const_cast<SOCKADDR_IN*>(&_RecvAddr), &AddrSize);
	oWSAVB(TotalReceived);
	return TotalReceived;
}

async_result accept_async(SOCKET _ListenSocket, SOCKET _AcceptSocket, void* _OutputBuffer, WSAOVERLAPPED* _pOverlapped)
{
	LPFN_ACCEPTEX pAcceptEx = getfn_AcceptEx(_ListenSocket);
	unsigned long bytesRead = 0;
	if (!pAcceptEx(_ListenSocket, _AcceptSocket, _OutputBuffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytesRead, _pOverlapped))
		return WSAGetLastError() == ERROR_IO_PENDING ? scheduled : failed;
	oASSERT(bytesRead == accept_buffer_size, "Should have read back the 2 addresses");
	return completed;
}

async_result acceptexsockaddrs_async(SOCKET _ListenSocket, void* _Buffer, LPSOCKADDR* _LocalAddr, LPINT _SzLocalAddr, LPSOCKADDR* _RemoteAddr, LPINT _SzRemoteAddr)
{
	LPFN_GETACCEPTEXSOCKADDRS pAcceptExSockAddrs = getfn_GetAcceptExSockaddrs(_ListenSocket);
	pAcceptExSockAddrs(_Buffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, _LocalAddr, _SzLocalAddr, _RemoteAddr, _SzRemoteAddr);
	return completed;
}

// Initialize winsock
class wsa_startup
{
public:
	wsa_startup()
	{
		WORD wVersion = MAKEWORD(2, 2);
		WSADATA wsaData;
		int err = WSAStartup(wVersion, &wsaData);
		if (err)
			oWSATHROW(err, "winsock 2.2 initialization failed: %s", as_string_WSAerr(err));
	}

	~wsa_startup()
	{
		WSACleanup();
	}
};
static wsa_startup sWSAStartup;

}}}
