#include "Socket.h"
#include <thread>

int main()
{
    int Num = std::thread::hardware_concurrency();

    const int MaxThreads = std::thread::hardware_concurrency();

    std::cout << "Enter Thread Count, Enter 0 for supported Max threads" << std::endl;
    std::cin >> Num;

	if (Num <= 0 or Num > MaxThreads)
		threadPool thpool(MaxThreads);
	else
		threadPool thpool(Num);

    exit(0);
    system("Pause");

    system("Echo Server Shutdown Successful");
}
