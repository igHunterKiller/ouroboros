// Copyright (c) 2016 Antony Arciuolo. See License.txt regarding use.

// Winsock is built on top of a sockets API. It is a complex and just awful API
// so this tries to wrap up some of the gory details to simplify usage. It's not 
// quite a socket interface, just enough to make implementing a socket interface 
// bearable on top of sockets and winsock.

#pragma once
#include <oString/fixed_string.h>
#include <functional>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <Mstcpip.h>
#include <Mswsock.h>
#include <nspapi.h>
#include <Rpc.h>
#include <Ws2tcpip.h>
#include <atomic>

#define oWSATHROW(_WSAErr, _Message, ...) do { throw std::system_error(ouro::windows::winsock::get_errc(_WSAErr), std::system_category(), stringf(_Message, ## __VA_ARGS__)); } while(false)
#define oWSATHROW0(_WSAErr) do { throw std::system_error(ouro::windows::winsock::get_errc(_WSAErr), std::system_category()); } while(false)
#define oWSAVB(_Expression) do { if (SOCKET_ERROR == (_Expression)) oWSATHROW(WSAGetLastError(), "%s", #_Expression); } while(false)
#define oWSAVBE(_Expression, _ExceptForResult) do { if (SOCKET_ERROR == (_Expression) && _ExceptForResult != WSAGetLastError()) oWSATHROW(WSAGetLastError(), "%s", #_Expression); } while(false)
#define oWSATHROWLAST() do { oWSATHROW0(WSAGetLastError()); } while(false)

namespace ouro {
	namespace windows {
		namespace winsock {

// From msdn docs, buffer needs 16 bytes padding for each address. buffer will 
// hold 2 addrs.
static const int accept_buffer_size = 2 * sizeof(SOCKADDR_IN) + 32; 

enum options
{
	reliable = 0x1, // if true the socket uses TCP, else UDP
	allow_broadcast = 0x2, // enable broadcast for UDP (ignored if reliable is specified)
	reuse_address = 0x4, // allows other processes to share the address 
	exclusive_address = 0x8, // ensures this socket has exclusive control of the specified address
	blocking = 0x10,
};

// Return value that describes if winsock completed the function synchronously. 
// (There is no iocp callback when this happens.)
enum async_result
{
	completed, // completed synchronously, there will be no iocp callback
	scheduled, // the operation is scheduled: expect an iocp callback
	failed, // the operation did not complete and there will be no iocp callback
};

// Return the define as a string
const char* as_string(int _WSAError);

// Return a bit more robust description of the error
const char* get_desc(int _WSAError);

// Return the std::errc for the specified WSA error
std::errc::errc get_errc(int _WSAError);

// These function calls must be initialized from a socket
LPFN_CONNECTEX getfn_ConnectEx(SOCKET _hSocket);
LPFN_DISCONNECTEX getfn_DisconnectEx(SOCKET _hSocket);
LPFN_GETACCEPTEXSOCKADDRS getfn_GetAcceptExSockaddrs(SOCKET _hSocket);
LPFN_TRANSMITPACKETS getfn_TransmitPackets(SOCKET _hSocket);
LPFN_WSARECVMSG getfn_WSARecvMsg(SOCKET _hSocket);
LPFN_ACCEPTEX getfn_AcceptEx(SOCKET _hSocket);

// Hostname is an address or name with a port (i.e. localhost:123 or 
// 127.0.0.1:123). For INADDR_ANY use IP 0.0.0.0 and any appropriate port.
sockaddr_in make_addr(const char* _Hostname);

char* addr_to_hostname(char* _StrDestination, size_t _SizeofStrDestination, const sockaddr_in& _SockAddr);
template<size_t size> char* addr_to_hostname(char (&_StrDestination)[size], const sockaddr_in& _SockAddr) { return addr_to_hostname(_StrDestination, size, _SockAddr); }
template<size_t capacity> char* addr_to_hostname(fixed_string<char, capacity>& _StrDestination, const sockaddr_in& _SockAddr) { return addr_to_hostname(_StrDestination, _StrDestination.capacity(), _SockAddr); }

// Fills the specified buffers with data from the specified socket. Null can be 
// specified for any one of these to opt out of getting a particular part of the
// data. Hostname is THIS part of the socket, the local port of connection. Peer
// name is the FOREIGN PART of the connection, meaning the ip/hostname and 
// foreign port of the connection.
void get_hostname(char* _StrOutHostname, size_t _SizeofStrOutHostname, char* _StrOutIPAddress, size_t _SizeofStrOutIPAddress, char* _StrOutPort, size_t _SizeofStrOutPort, SOCKET _hSocket);
template<size_t hostnameSize, size_t ipSize, size_t portSize> inline void get_hostname(char (&_OutHostname)[hostnameSize], char (&_OutIPAddress)[ipSize], char (&_OutPort)[portSize], SOCKET _hSocket) { get_hostname(_StrOutHostname, hostnameSize, _OutIPAddress, ipSize, _StrOutPort, portSize, _hSocket); }

void get_peername(char* _StrOutHostname, size_t _SizeofStrOutHostname, char* _StrOutIPAddress, size_t _SizeofStrOutIPAddress, char* _StrOutPort, size_t _SizeofStrOutPort, SOCKET _hSocket);
template<size_t hostnameSize, size_t ipSize, size_t portSize> inline void get_peername(char (&_OutHostname)[hostnameSize], char (&_OutIPAddress)[ipSize], char (&_OutPort)[portSize], SOCKET _hSocket) { get_peername(_StrOutHostname, hostnameSize, _StrOutIPAddress, ipSize, _StrOutPort, portSize, _hSocket); }

// For when creating a winsock with automatic port selection get the 
// selected port.
unsigned short get_port(SOCKET _hSocket);

// Returns true if a poll reveals POLLHUP as not set.
bool connected(SOCKET _hSocket);

// Returns true if a poll reveals POLLRDNORM or POLLWRNORM as set.
bool connected2(SOCKET _hSocket);

// Enumerates the addresses of all attached interfaces
void enumerate_addresses(const std::function<void(const sockaddr_in& _Addr)> _Enumerator);

// Returns false if timed out, otherwise this is just a thin wrapper around 
// WSAWaitForMultipleEvents to make it more consistent with other API calls.
bool wait_multiple(WSAEVENT* _phEvents, size_t _NumberOfEvents, bool _WaitAll, unsigned int _TimeoutMS);
inline bool wait(WSAEVENT _hEvent, unsigned int _TimeoutMS) { return wait_multiple(&_hEvent, 1, true, _TimeoutMS); }

// Create a winsock socket that can support overlapped operations.
// _Hostname: IP/DNS colon the port number (127.0.0.1:123 or myhostname:321)
// _Options: OR'ed together winsock::options values.
// _TimeoutMS: Number of milliseconds to wait for this socket to connect. This 
//             value is ignored if the socket is started as a server/listener.
// _MaxNumConnections: If 0 the socket will be created and connect() will be 
//                     called. If non-zero the socket will be bind()'ed and put 
//                     into a listen state as a server. If negative, then 
//                     SOMAXCONN will be used.
SOCKET make(const sockaddr_in& _Addr, int _Options, unsigned int _TimeoutMS = 0, unsigned int _MaxNumConnections = 0);
SOCKET make(const sockaddr_in6& _Addr, int _Options, unsigned int _TimeoutMS = 0, unsigned int _MaxNumConnections = 0);

// For use with AcceptEx (iocp), this make creates an unconnected socket ready 
// to be sent to AcceptEx. It is not usable at this point. Once iocp accepts a 
// connection with this socket, you must call finish_async_accept to finish 
// prepping the socket for use.
SOCKET make_async_accept();
void finish_async_accept(SOCKET _hListenSocket, SOCKET _hAcceptSocket);

// This will disconnect the socket but not destroy it. It will be in the same 
// state as a socket returned from make_async_accept. This is intended to 
// improve server performance so a socket doesn't have to be deleted and 
// recreated. This may or may not complete asynchronously. Remember to respond
// appropriate to the returned values because if this returns synchronously an 
// iocp event will not occur.
async_result recycle_async_accept(SOCKET _hListenSocket, SOCKET _hAcceptSocket, WSAOVERLAPPED* _pOverlapped);

// Thoroughly closes a socket according to best-practices described on the web.
void close(SOCKET _hSocket);

// Default is time is a 2 hour timeout with a 1 second interval.
void set_keepalive(SOCKET _hSocket, unsigned int _TimeoutMS = 2 * 60 * 60 * 1000, unsigned int _IntervalMS = 1000);

void send(SOCKET _hSocket, const void* _pSource, size_t _SizeofSource, const sockaddr_in* _pDestination = nullptr);

// Returns number of bytes read. This may include more than one send()'s worth
// of data due to Nagel's Algorithm. This can throw std::errc::connection_reset 
// meaning a valid and error-free closing of the peer socket has occurred and no 
// further steps should occur. _pInOutCanReceive is the address of an integer 
// used as a boolean (atomics used to change its state) that is 0 if receive 
// should not wait because of a bad _hSocket, or non-zero if the receive should 
// block on FD_READ events.
size_t receive(SOCKET _hSocket, WSAEVENT _hEvent, void* _pDestination, size_t _SizeofDestination, unsigned int _TimeoutMS, std::atomic<int>* _pInOutCanReceive, sockaddr_in* _pSource);

// Returns number of bytes read. This can throw std::errc::connection_reset 
// meaning a valid and error-free closing of the peer socket has occurred and no 
// further steps should occur.
size_t receive_nonblocking(SOCKET _hSocket, WSAEVENT _hEvent, void* _pDestination, size_t _SizeofDestination, sockaddr_in* _pSource);

// Returns number of bytes read.
size_t recvfrom_blocking(SOCKET _hSocket, void* _pDestination, size_t _SizeofDestination, unsigned int _TimeoutMS, const SOCKADDR_IN& _RecvAddr, unsigned int _recvfromFlags = 0);
	
// _OutputBuffer must be at least accept_buffer_size in size.
async_result accept_async(SOCKET _ListenSocket, SOCKET _AcceptSocket, void* _OutputBuffer, WSAOVERLAPPED* _pOverlapped);

// This function parses the _OutputBuffer obtained from a call to the 
// accept_async function and passes the local and remote addresses to a 
// sockaddr structure.
async_result acceptexsockaddrs_async(SOCKET _ListenSocket, void* _Buffer, LPSOCKADDR* _LocalAddr, LPINT _SzLocalAddr, LPSOCKADDR* _RemoteAddr, LPINT _SzRemoteAddr);

}}}
