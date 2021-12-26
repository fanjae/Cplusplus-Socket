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

// ������ �����ϴ� ������.
Socket::Socket(SocketType socketType)
{
	g_socketInit.Touch();

	// overlapped I/O�� ������ socket() ���� WSASocket() �ʿ�.
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

// �ܺ� ���� �ڵ��� �޴� ������.
Socket::Socket(SOCKET fd)
{
	g_socketInit.Touch();
	m_fd = fd;

#ifdef _WIN32
	ZeroMemory(&m_readOverlappedStruct, sizeof(m_readOverlappedStruct));
#endif
}

// ������ ���������� �ʴ´�.
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

// Endpoint�� ����Ű�� �ּҷ� ����.
void Socket::Connect(const Endpoint& endpoint)
{
	if (connect(m_fd, (sockaddr*)&endpoint.m_ipv4Endpoint, sizeof(endpoint.m_ipv4Endpoint)) < 0)
	{
		stringstream ss;
		ss << "connect failed : " << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
}

// �۽��� �մϴ�.
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

// �����ϸ� 0, �����ϸ� �ٸ� �� ����.
// errorText���� ���н� ���� ������ �ؽ�Ʈ�� ä����.
// acceptedSocket���� accept�� ���� �ڵ��� ��.
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

#ifdef _WIN32

bool Socket::AcceptOverlapped(Socket& acceptCandidateSocket, string& errorText)
{
	if (AcceptEx == NULL)
	{
		DWORD bytes;
		// AcceptEx�� ��Ÿ �����Լ��� �޸� ���� ȣ���ϴ� ���� �ƴϰ�,
		// �Լ� �����͸� ���� ������ ���� ȣ���� �� �ִ�. �װ��� ���⼭ �Ѵ�.
		WSAIoctl(m_fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &UUID(WSAID_ACCEPTEX), sizeof(UUID), &AcceptEx, sizeof(AcceptEx), &bytes, NULL, NULL);

		if (AcceptEx = NULL)
		{
			throw Exception("Getting AcceptEx ptr failed.");
		}
	}

	// ���⿡�� accept�� ������ �����ּҿ� ����Ʈ�ּҰ� ä������.
	char ignored[200];
	DWORD ignored2 = 0;

	bool ret = AcceptEx(m_fd, acceptCandidateSocket.m_fd, &ignored, 0, 50, 50, &ignored2, &m_readOverlappedStruct) == TRUE;

	return ret;
}

// AcceptEx�� I/O �ϷḦ �ϴ��� ���� TCP ���� �ޱ� ó���� �� ���� ���� �ƴϴ�.
// �� �Լ��� ȣ�����־�߸� �Ϸ�ȴ�.
int Socket::UpdateAcceptContext(Socket& listenSocket)
{
	sockaddr_in ignore1;
	sockaddr_in ignore3;
	INT ignore2, ignore4;

	char ignore[1000];
	GetAcceptExSockaddrs(ignore, 0, 50, 50, (sockaddr**)&ignore1, &ignore2, (sockaddr**)&ignore3, &ignore4);

	return setsockopt(m_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char *)&listenSocket.m_fd, sizeof(listenSocket.m_fd));
}

#endif // _WIN32 

// EndPoint �ּ� get
Endpoint Socket::GetPeerAddr()
{
	Endpoint ret;
	socklen_t retLength = sizeof(ret.m_ipv4Endpoint);
	if (::getpeername(m_fd, (sockaddr*)&ret.m_ipv4Endpoint, &retLength) < 0)
	{
		stringstream ss;
		ss << "getPeerAddr failed: " << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
	if (retLength > sizeof(ret.m_ipv4Endpoint))
	{
		stringstream ss;
		ss << "getPeerAddr buffer overrun : " << retLength;
		throw Exception(ss.str().c_str());
	}

	return ret;
}


