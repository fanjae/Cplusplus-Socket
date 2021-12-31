#include "stdafx.h"
#include <iostream>
#include "../ImaysNet/ImaysNet.h"
#include <chrono>
#include <thread>

#pragma comment (lib, "./ImaysNet.lib")

using namespace std;

int main()
{
	try
	{
		Socket s(SocketType::Tcp); // 1
		s.Bind(Endpoint::Any);     // 2
		cout << "Connecting to server...\n";
		s.Connect(Endpoint("127.0.0.1", 5959)); // 3

		// 4
		cout << "Sending data...\n";
		const char* text = "hello";

		// 보낼 문자열의 sz도 포함해서 전송해야 하므로 +1을 추가.
		s.Send(text, strlen(text) + 1);

		std::this_thread::sleep_for(1s);

		// 5
		cout << "Closing socket...\n";
		s.Close();
	}
	catch (Exception& e)
	{
		cout << "Exception! " << e.what() << endl;
	}
	return 0;
}

