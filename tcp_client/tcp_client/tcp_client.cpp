#include "stdafx.h"
#include <iostream>
#include "../../Networklibrary/Networklibrary/ImaysNet.h"
#include <chrono>
#include <thread>

using namespace std;

int main()
{
	try
	{
		Socket s(SocketType::Tcp); // 1
		s.Bind(Endpoint::Any);
		cout << "Connecting to server...\n";
		s.Connect(Endpoint("127.0.0.1", 5959)); // IP Address & Port

		// 4
		cout << "Sending data...\n";
		const char *text = "hello";
		// ���� ���ڿ��� sz�� �����ؼ� �����ؾ� �ϹǷ� +1 �մϴ�.
		s.Send(text, strlen(text) + 1);

		// �� �۽� �Լ��� �����ϴ��� �̰��� ���Ͽ� �۽� �����͸� ���� ���� ��,
		// ���� ������ �����ߴٴ� ���� �ƴ�.
		// �� ���¿��� ���� �ݱ⸦ �ٷ� �ع����� ������ �����͸� ���� ���ϴ� ���ɼ��� �ִ�.
		// ���� �ǵ������� ������ ���� �� �ִ� ����� �ð��� ��ٸ�.
		// ���� ���α׷�.
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
