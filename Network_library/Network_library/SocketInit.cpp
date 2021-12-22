#include "SocketInit.h"
#include <iostream>

using namespace std;

SocketInit g_socketInit;

SocketInit::SocketInit()
{
#ifdef _WIN32

	WSADATA w;
	WSAStartup(MAKEWORD(2, 2), &w);
#endif

}

void SocketInit::Touch()
{
}

