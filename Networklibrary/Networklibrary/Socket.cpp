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

#ifdef _WIN32

bool Socket::AcceptOverlapped(Socket& acceptCandidateSocket, string& errorText)
{
	if (AcceptEx == NULL)
	{
		DWORD bytes;
		// AcceptEx는 여타 소켓함수와 달리 직접 호출하는 것이 아니고,
		// 함수 포인터를 먼저 가져온 다음 호출할 수 있다. 그것을 여기서 한다.
		WSAIoctl(m_fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &UUID(WSAID_ACCEPTEX), sizeof(UUID), &AcceptEx, sizeof(AcceptEx), &bytes, NULL, NULL);

		if (AcceptEx = NULL)
		{
			throw Exception("Getting AcceptEx ptr failed.");
		}
	}

	// 여기에는 accept된 소켓의 로컬주소와 리모트주소가 채워진다.
	char ignored[200];
	DWORD ignored2 = 0;

	bool ret = AcceptEx(m_fd, acceptCandidateSocket.m_fd, &ignored, 0, 50, 50, &ignored2, &m_readOverlappedStruct) == TRUE;

	return ret;
}

// AcceptEx가 I/O 완료를 하더라도 아직 TCP 연결 받기 처리가 다 끝난 것이 아니다.
// 이 함수를 호출해주어야만 완료된다.
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

// 소켓 수신을 합니다. 
// 블로킹 소켓이면 1바이트라도 수신하거나 소켓 에러 또는 소켓 연결이 끊어질때까지 기다림
// 논블로킹 소켓이면 기다려야 하는 경우 즉시 리턴, EWOULDBLOCK이 errno나 GetLastError에서 나오게 됨.
// 리턴값: recv 리턴값 그대로입니다.
int Socket::Receive()
{
	return (int)recv(m_fd, m_receiveBuffer, MaxReceiveLength, 0);
}

#ifdef _WIN32

// overlapped 수신을 검. 즉, 백그라운드로 수신 처리.
// 수신 되는 데이터는 m_receiveBuffer에 비동기로 채워짐.
// 리턴값 : WSARecv의 리턴값 그대로.

int Socket::ReceiverOverlapped()
{
	WSABUF b;
	b.buf = m_receiveBuffer;
	b.len = MaxReceiveLength;

	// overlapped I/O가 진행되는 동안 여기에 값이 채워짐.
	m_readFlags = 0;

	return WSARecv(m_fd, &b, 1, NULL, &m_readFlags, &m_readOverlappedStruct, NULL);
}

#endif

// NonBlock Socket으로 모드를 설정한다.
void Socket::SetNonblocking()
{
	u_long val = 1;
#ifdef _WIN32
	int ret = ioctlsocket(m_fd, FIONBIO, &val);
#else
	int ret = ioctl(m_fd, FIONBIO, &val);
#endif
	if (ret != 0)
	{
		stringstream ss;
		ss << "bind failed: " << GetLastErrorAsString();
		throw Exception(ss.str().c_str());
	}
}

// Win32 오류를 문자열 형식으로 반환
// 오류 없는 경우 빈 문자열 반환
// 출처: https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror

std::string GetLasterrorAsString()
{
#ifdef _WIN32
	// Get the error Message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string();

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	// Free the Buffer.
	LocalFree(messageBuffer);

#else
	std::string message = strerror(errno);
#endif
	return message;
}



