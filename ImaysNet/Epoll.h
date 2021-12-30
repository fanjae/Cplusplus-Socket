#pragma once

#ifndef _WIN32
#include <unistd.h>
#include <sys/epoll.h>

class Socket;
class EpollEvents;

// Linux Epoll ��ü.
class Epoll
{
public:
	// 1ȸ�� epoll_wait�� �ִ��� ������ �� �ִ� ���� ����
	static const int MaxEventCount = 1000;

	int m_epollFd;

	Epoll();
	~Epoll();

	void Add(Socket& socket, void* userPtr, int eventFlags);
	void Wait(Epollevents& output, int timeoutMs);
};

// epoll_wait���� ������ �̺�Ʈ��
class EpollEvents
{
public:
	// epoll_wait���� ������ �̺�Ʈ��
	epoll_event m_events[Epoll::MaxEventCount];
	int m_eventcount;
};

#endif // !_WIN32
