#include "stdafx.h"
#include "../ImaysNet/ImaysNet.h"
#pragma comment (lib, "ws2_32.lib")
#pragma comment (lib, "./ImaysNet.lib")

using namespace std;

int main()
{
	try
	{
		Socket listenSocket(SocketType::Tcp); // 1
		listenSocket.Bind(Endpoint("127.0.0.1", 5959)); // 2
		listenSocket.Listen(); // 3
		cout << "Server started.\n";

		// 4
		Socket tcpConnection;
		string ignore;
		listenSocket.Accept(tcpConnection, ignore);

		// 5
		auto a = tcpConnection.GetPeerAddr().ToString();
		cout << "Socket from " << a << "is accepted.\n";
		while (true)
		{
			// 6
			string receivedData;
			cout << "Receiving data...\n";
			int result = tcpConnection.Receive();

			// 7
			if (result == 0)
			{
				cout << "Connection closed.\n";
				break;
			}
			else if (result < 0)
			{
				cout << "Connect lost : " << GetLastErrorAsString() << endl;
			}

			// 수신된 데이터를 꺼내서 출력한다. 송신자는 "hello\0"를 보냈으므로 sz가 있음을 그냥 믿고 출력함.
			// 실제에서는 클라이언트가 보낸 데이터는 그냥 믿으면 안됨.
			cout << "Received : " << tcpConnection.m_receiveBuffer << endl;
		}
		tcpConnection.Close(); // 8
	}
	catch (Exception& e)
	{
		cout << "Exception! " << e.what() << endl;
	}
	return 0;
}