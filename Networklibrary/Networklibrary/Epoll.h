#pragma once

#ifndef _WIN32
#include <unistd.h>
#include <sys/epoll.h>

class Socket;
class EpollEvents;

// Linux Epoll 객체.
class Epoll
{
public:
	// 1회의 epoll_wait이 최대한 꺼내올 수 있는 일의 개수
	static const int MaxEventCount = 1000;

	int m_epollFd;

	Epoll();
	~Epoll();

	void Add(Socket& socket, void* userPtr, int eventFlags);
	void Wait(Epollevents& output, int timeoutMs);
};

// epoll_wait으로 꺼내온 이벤트들
class EpollEvents
{
public:
	// epoll_wait으로 꺼내온 이벤트들
	epoll_event m_events[Epoll::MaxEventCount];
	int m_eventcount;
};

#endif // !_WIN32
