#include "stdafx.h"
#include "../ImaysNet/ImaysNet.h"
#include <stdio.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <signal.h>
#include <memory>
#include <vector>
#include <deque>
#include <iostream>
#include <mutex>

#pragma comment (lib, "./ImaysNet.lib")

using namespace std;

// TCP 연결 각각의 객체.
struct RemoteClient
{
	shared_ptr <thread> thread; // Client 처리를 하는 스레드 1개
	Socket tcpConnection; // accept한 TCP 연결
};

unordered_map<RemoteClient *, shared_ptr<RemoteClient>> remoteClients; // 여기 들어온 TCP 연결 객체들. key는 RemoteClient의 포인터값 자체이다.
deque <shared_ptr<RemoteClient>> remoteClientDeathNote; // 살생부. 여기에 들어간 RemoteClient는 곧 파괴된다.
Semaphore mainThreadWorkCount; // 여기에 +1이 발생하면 뭔가 할일이 있다는 뜻. 메인 스레드가 깨어나고 일을 한다.

// 위의 변수를 보호하는 mutex. 메인 스레드와 TCP 연결 받기를 처리하는 스레드가 공유하기 때문에 필요하다.
recursive_mutex remoteClientsMutex;
recursive_mutex consoleMutex; // 콘솔 출력을 여러 스레드가 하면 꼬일 수 있다. 그래서 처리를 일렬로한다.

volatile bool stopWorking = false;

shared_ptr <Socket> listenSocket; // 리스닝 소켓

void RemoteclientThread(shared_ptr<RemoteClient> remoteclient);
void ListenSocketThread();

void ProcessSignalAction(int sig_number)
{
	if (sig_number == SIGINT)
	{
		stopWorking = true;
		mainThreadWorkCount.Notify();
	}
}
int main()
{

	/* 실제에서는 사용하지 않으나, 여러 스레드가 CPU 자원을 잡아먹음으로 인해서 컴퓨터가 먹통이 되는 문제 발생.
	// 따라서 프로세스의 우선순위를 낮춰서 실험을 진행함.
	*/

	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	try
	{
		// 사용자가 Ctl-C를 누르면 메인루프 종료되도록 설정.
		signal(SIGINT, ProcessSignalAction);

		// 클라이언트 연결 수 만큼 스레드를 생성하는 Blocking Socket 방식

		// 많은 수의 클라이언트가 tcp 5555로 연결 들어옴.
		// 동일하게 Hello World를 보낼 것.
		// 이를 그냥 Echo 해둠.
		// 서버에서는 총 처리한 바이트 수를 지속적으로 출력 함.

		// TCP 연결을 받는 소켓
		listenSocket = make_shared<Socket>(SocketType::Tcp);
		listenSocket->Bind(Endpoint("0.0.0.0", 5555));
		listenSocket->Listen();

		// 리스닝 소켓을 위한 스레드를 생성한다.
		thread tcpListenThread(ListenSocketThread);

		while (!stopWorking)
		{
			// 메인 스레드에서 할 일이 있을 때까지 기다린다.
			mainThreadWorkCount.Wait();

			// 살생부 처리
			lock_guard<recursive_mutex> lock(remoteClientsMutex);

			while (!remoteClientDeathNote.empty())
			{
				auto remoteClientToDelete = remoteClientDeathNote.front();
				remoteClientToDelete->tcpConnection.Close();
				remoteClientToDelete->thread->join();
				remoteClients.erase(remoteClientToDelete.get()); // 클라이언트 목록에서 제거한다.
				remoteClientDeathNote.pop_front(); // Socket close도 이 과정에서 이루어진다.

				lock_guard<recursive_mutex> lock(consoleMutex);
				cout << "Client left. There are " << remoteClients.size() << " connections.\n";
			}
		}

		// 메인 루프 종료. 프로그램을 종료해야하는 상황.

		// 접속해 있는 모든 클라이언트 소켓도 닫음.
		listenSocket->Close();
		{
			lock_guard<recursive_mutex> lock(remoteClientsMutex);
			for (auto i : remoteClients)
			{
				// 모든 스레드를 종료시킨다.
				i.second->tcpConnection.Close();
				i.second->thread->join();
			}
		}

		// 모든 스레드를 종료시킨다.
		tcpListenThread.join();

		// 스레드들이 모두 종료할때까지 기다리는 처리도 포함된다.
		remoteClients.clear();
	}
	catch (Exception& e)
	{
		cout << "Exception! " << e.what() << endl;
	}

	return 0;
}

// TCP 소켓 연결을 받는 처리를 하는 스레드
void ListenSocketThread()
{
	while (!stopWorking)
	{
		auto remoteClient = make_shared<RemoteClient>();
		// TCP 연결이 들어옴을 기다린다.
		string errorText;
		if (listenSocket->Accept(remoteClient->tcpConnection, errorText) == 0)
		{
			// TCP 연결이 들어왔다.
			// RemoteClient 객체를 만듦.
			{
				lock_guard<recursive_mutex> lock(remoteClientsMutex);
				remoteClients.insert({ remoteClient.get(), remoteClient });
			}

			// 콘솔 출력
			{
				lock_guard<recursive_mutex> lock(consoleMutex);
				cout << "Client joined. There are " << remoteClients.size() << "connections. \n";
			}
		}
		else
		{
			// accept이 실패했다. 리스닝 스레드를 종료하자.
			break;
		}
	}
}

// 이미 들어온 TCP 연결을 처리하는 스레드
void RemoteclientThread(shared_ptr<RemoteClient> remoteClient)
{
	while (!stopWorking)
	{
		// 받은 데이터를 그대로 회신한다.
		int ec = remoteClient->tcpConnection.Receive();
		if (ec <= 0)
		{
			if (ec <= 0) // 개인 주석 : 왜 동일 조건을 두번 넣었을까?
			{
				// 에러 혹은 소켓 종료이다.
				break;
			}
		}

		// TCP 스트림 특성상 일부만 송신하고 리턴하는 경우도 고려해야함.
		remoteClient->tcpConnection.Send(remoteClient->tcpConnection.m_receiveBuffer, ec);
	}

	// 루프 종료. 이 RemoteClient는 종료되어야 함을 살생부에 남기자.
	remoteClient->tcpConnection.Close();
	lock_guard<recursive_mutex> lock(remoteClientsMutex);
	remoteClientDeathNote.push_back(remoteClient);
	mainThreadWorkCount.Notify();
}


