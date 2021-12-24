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
typedef int SOCKET; // SOCKET�� 64bit ȯ�濡�� 64bit�̳�, Linux�� ��� 32bit.
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

	SOCKET m_fd; // ���� Handle

#ifdef _WIN32

	// AcceptEx �Լ� ������
	LPFN_ACCEPTEX AcceptEx = NULL;

	// Overlapped I/O�� OCP�� �� ���� ���.
	// Overlapped I/O�� ��� ���� ��� true����.
	bool m_isReadOverlapped = false;

	// Overlapped receive or accept�� �� �� ���Ǵ� overlapped ��ü.
	// I/O �Ϸ� �������� �����Ǿ�� �Ѵ�.
	WSAOVERLAPPED m_readOverlappedStruct;
#endif


	// Receive�� ReceiverOverlapped�� ���� ���ŵǴ� �����Ͱ� ä������ ��.
	// Overlapped Receive�� �ϴ� ���� �̰��� ���. Overlapped�� ����Ǵ� ���� ���� �ǵ帮�� �� ��.
	char m_receiverBuffer[MaxReceiveLength];

#ifdef _WIN32
	// overlapped ������ �ϴ� ���� ���⿡ recv�� flags�� ���ϴ� ���� ä���� overlapped I/O�� ����Ǵ� ���� �ǵ帮�� �� ��.
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

