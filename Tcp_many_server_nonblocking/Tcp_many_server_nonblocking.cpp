#include "stdafx.h"
#include "../ImaysNet/ImaysNet.h"
#include <stdio.h>
#include <signal.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <memory>
#include <vector>
#include <iostream>
#include <mutex>

using namespace std;

// true가 되면 프로그램을 종료함.
volatile bool stopWorking = false;

void ProcessSignalAction(int sig_number)
{
	if (sig_number == SIGINT)
	{
		stopWorking = true;
	}
}

int main()
{
	// 사용자가 Ctrl-C를 누르면 메인루프를 종료하게 만든다.
	signal(SIGINT, ProcessSignalAction);

	try
	{
		// Non BlockSocket으로 많은 수의 클라이언트를 받아 처리함.

		// TCP 연결 각각의 객체
		struct RemoteClient
		{
			Socket tcpConnection; // Accept한 TCP 연결
		};
		unordered_map<RemoteClient*, shared_ptr<RemoteClient>> remoteClients;

		// TCP 연결을 받는 소켓
		Socket listenSocket(SocketType::Tcp);
		listenSocket.Bind(Endpoint("0.0.0.0", 5555));

		listenSocket.SetNonblocking();
		listenSocket.Listen();

		cout << "서버가 시작되었습니다.\n";
		cout << "Ctrl-C키를 누르면 프로그램을 종료합니다.\n";

		// 리슨 소켓과 TCP 연결 소켓 모두에 대해서 I/O 가능(avail) 이벤트가 있을 때 까지 기다림.
		// I/O 가능 소켓에 대한 일을 처리함.

		// 아래 넣은 소켓 핸들에 대해서 select나 poll을 한다.
		// Receive나 Accept에 대해서만 처리한다.

		vector <PollFD> readFds;

		// 어느 소켓이 어느 RemoteClient에 대한 것인지를 가리킵니다.
		vector <RemoteClient*> readFdsToRemoteClients;

		while (!stopWorking)
		{

		}
	}
}
