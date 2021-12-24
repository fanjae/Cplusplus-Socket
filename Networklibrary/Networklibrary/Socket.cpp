#include "stdafx.h"
#ifdef _WIN32
#include <rpc.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#endif

#include "Socket.h"
#include "Endpoint.h"
#include "SocketInit.h"
#include "Exception.h"

using namespace std;

std::string GetLasterrorAsString();

// 소켓을 생성하는 생성자.
Socket::Socket(SocketType socketType)
{
	g_socketInit.Touch();

	// overlapped I/O를 쓰려면 socket() 말고 WSASocket() 필요.
	if (socketType == SocketType::Tcp)
	{
#ifdef _WIN32
		m_fd = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
		m_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif
	}
	else
	{
#ifdef _WIN32
		m_fd = WSASocket(AF_INET, SOCK_DGRAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
#else
		m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#endif
	}
#ifdef _WIN32
	ZeroMemory(&m_readOverlappedStruct, sizeof(m_readOverlappedStruct));
#endif
}

// 외부 소켓 핸들을 받는 생성자.
Socket::Socket(SOCKET fd)
{
	g_socketInit.Touch();
	m_fd = fd;

#ifdef _WIN32
	ZeroMemory(&m_readOverlappedStruct, sizeof(m_readOverlappedStruct));
#endif
}

// 소켓을 생성하지는 않는다.
Socket::Socket()
{
#ifdef _WIN32
	static_assert(INVALID_SOCKET == -1, "");
#endif

	m_fd = -1;

#ifdef _WIN32
	ZeroMemory(&m_readOverlappedStruct, sizeof(m_readOverlappedStruct));
#endif
}

Socket::~Socket()
{
	Close();
}

void Socket::Bind(const Endpoint& endpoint)
{
	if (bind(m_fd, (sockaddr*)&endpoint.m_ipv4Endpoint, sizeof(endpoint.m_ipv4Endpoint)) < 0)
	{
		stringstream ss;
		ss << "bind failed: " << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
}

// Endpoint가 가리키는 주소로 접속.
void Socket::Connect(const Endpoint& endpoint)
{
	if (connect(m_fd, (sockaddr*)&endpoint.m_ipv4Endpoint, sizeof(endpoint.m_ipv4Endpoint)) < 0)
	{
		stringstream ss;
		ss << "connect failed : " << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
}

// 송신을 합니다.
int Socket::Send(const char *data, int length)
{
	return ::send(m_fd, data, length, 0);
}

void Socket::Close()
{
#ifdef _WIN32
	closesocket(m_fd);
#else
	close(m_fd);
#endif
}

void Socket::Listen()
{
	listen(m_fd, 5000);
}

// 성공하면 0, 실패하면 다른 값 리턴.
// errorText에는 실패시 에러 내용이 텍스트로 채워짐.
// acceptedSocket에는 accept된 소켓 핸들이 들어감.
int Socket::Accept(Socket & acceptedSocket, string& errorText)
{
	acceptedSocket.m_fd = accept(m_fd, NULL, 0);
	if (acceptedSocket.m_fd == -1)
	{
		errorText = GetLastErrorAsString();
		return -1;
	}
	else
	{
		return 0;
	}
}




