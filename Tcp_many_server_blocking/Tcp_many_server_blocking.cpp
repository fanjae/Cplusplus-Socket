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

using namespace std;

// TCP 연결 각각의 객체.
struct RemoteClient
{
	shared_ptr <thread> thread; // Client 처리를 하는 스레드 1개
	Socket tcpConnect; // accept한 TCP 연결
};

unordered_map<RemoteClient *, shared_ptr<RemoteClient>> remoteClients; // 여기 들어온 TCP 연결 객체들. key는 RemoteClient의 포인터값 자체이다.
deque <shared_ptr<RemoteClient>> remoteclientDeathNote; // 살생부. 여기에 들어간 RemoteClient는 곧 파괴된다.
Semaphore mainThreadWorkCount; // 여기에 +1이 발생하면 뭔가 할일이 있다는 뜻. 메인 스레드가 깨어나고 일을 한다.

// 위의 변수를 보호하는 mutex. 메인 스레드와 TCP 연결 받기를 처리하는 스레드가 공유하기 때문에 필요하다.
recursive_mutex remoteclientsMutex;
recursive_mutex consoleMutex; // 콘솔 출력을 여러 스레드가 하면 꼬일 수 있다. 그래서 처리를 일렬로 줄 세우자.

volatile bool stopWorking = false;

shared_ptr <Socket> listenSocket; // 리스닝 소켓

void RemoteclientThread(shared_ptr<RemoteClient> remoteclient);
void ListenSocketThread();

void ProcessSignalAction(int sig_number)
{
	if (sig_number == SIGINT)
	{
		stopWorking = true;
		mainthreadWorkCount.Notify();
	}
}
