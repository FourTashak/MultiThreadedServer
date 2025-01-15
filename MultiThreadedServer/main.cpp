#include "Socket.h"

int main()
{
    int Num = std::thread::hardware_concurrency();

    const int MaxThreads = std::thread::hardware_concurrency();

    std::cout << "Enter Thread Count, Enter 0 for supported Max threads" << std::endl;
    std::cin >> Num;

    ThreadPoolManager* Manager;

	if (Num <= 0 or Num > MaxThreads)
    {
        Manager = new ThreadPoolManager(MaxThreads);
    }
	else
    {
        Manager = new ThreadPoolManager(Num);
    }

    exit(0);
    system("Pause");

    system("Echo Server Shutdown Successful");
}
