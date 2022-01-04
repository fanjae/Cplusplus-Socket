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

