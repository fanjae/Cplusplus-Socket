#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#include <windows.h>
#include <MSWSock.h>
#else
#include <sys/socket.h>
#endif

#include <string>

#ifndef _WIN32
typedef int SOCKET; // SOCKET은 64bit 환경에서 64bit이나, Linux에 경우 32bit.
#endif

class Endpoint;

enum class SocketType
{
	Tcp,
	Udp,
};

class Socket
{
public:
	static const int MaxReceiveLength = 8192;

	SOCKET m_fd; // 소켓 Handle

#ifdef _WIN32

	// AcceptEx 함수 포인터
	LPFN_ACCEPTEX AcceptEx = NULL;

	// Overlapped I/O나 OCP를 쓸 때만 사용.
	// Overlapped I/O를 사용 중인 경우 true상태.
	bool m_isReadOverlapped = false;

	// Overlapped receive or accept를 할 때 사용되는 overlapped 객체.
	// I/O 완료 전까지는 보존되어야 한다.
	WSAOVERLAPPED m_readOverlappedStruct;
#endif


	// Receive나 ReceiverOverlapped에 의해 수신되는 데이터가 채워지는 곳.
	// Overlapped Receive를 하는 동안 이곳이 사용. Overlapped가 진행되는 동안 값을 건드리지 말 것.
	char m_receiverBuffer[MaxReceiveLength];

#ifdef _WIN32
	// overlapped 수신을 하는 동안 여기에 recv의 flags에 준하는 값이 채워짐 overlapped I/O가 진행되는 동안 건드리지 말 것.
	DWORD m_readFlags = 0;
#endif

	Socket();
	Socket(SOCKET fd);
	Socket(SocketType socketType);
	~Socket();

	void Bind(const Endpoint& endpoint);
	void Connect(const Endpoint& endpoint);
	int Send(const char* data, int length);
	void Close();
	void Listen();
	int Accept(Socket& acceptedSocket, std::string& errorText);
#ifdef _WIN32
	bool AcceptOverlapped(Socket& acceptCandidateSocket, std::string& errorText);
	int UpdateAcceptContext(Socket& listenSocket);
#endif
	Endpoint GetPeerAddr();
	int Receive();
#ifdef _WIN32
	int ReceiverOverlapped();
#endif
	void SetNonblocking();
};

std::string GetLastErrorAsString();

#ifdef _WIN32
#pragma comment(lib,"ws2_32.lib");
#pragma comment(lib,"mswsock.lib");
#endif

