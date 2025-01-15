#pragma once
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <string>
#include <conio.h>
#include <thread>
#include <atomic>
#include <chrono>

#pragma comment (lib, "Ws2_32.lib")

class Work;
class WorkerThread;
class ThreadPoolManager;
class Connections;

class Work
{
public:

    Work(){}
    Work(std::vector<int>& InWork, int InOwnerIndex, SOCKET& SenderSock) 
        : Numbers(InWork), OwnerThreadIndex(InOwnerIndex), QuerierSocket(&SenderSock) {}

    std::vector<int> Numbers;

    int OwnerThreadIndex = 0;

    SOCKET* QuerierSocket{};

    std::atomic<bool> BTaken{false};

    int64_t Result = 0;

    bool BMarkedForDelete = false;
};

class Connections
{
public:
    Connections(std::unique_ptr<SOCKET> Socket, sockaddr_in clientaddress)
    {
        sock_.reset(Socket.release());
        Clientaddress_ = clientaddress;
    }
    ~Connections()
    {
        closesocket((SOCKET)sock_.get());
    }

    std::shared_ptr<SOCKET> sock_;
    sockaddr_in Clientaddress_;
    std::string Name;
};

class WorkerThread
{
public:
    WorkerThread(int threadIndex,std::vector<std::vector<Connections>>& ConVec, std::vector<fd_set>& ReadVec, ThreadPoolManager* MainPoolPtr);

    //this is the main function for the worker threads, here the worker threads are in an infinite loop where they check on sockets assigned to them
    //and receive data if there is data to be received.
    void run(int number, std::vector<std::vector<Connections>>& ConVec, std::vector<fd_set>& ReadVec);

    bool JoinFlag = false;

    void GetWorkAndSendResult(Work* InWork);

    ThreadPoolManager* OwnerPool;

    std::thread* ResponsibleOfThread = nullptr;
};

class ThreadPoolManager
{
private:
    std::vector<bool> JoinFlag;

    std::vector<fd_set> Readsets;
    std::vector<std::vector<Connections>> Cons;

    std::vector<Work*> WorkToBeDone;

    std::chrono::steady_clock::time_point LastTime;

    std::vector<WorkerThread*> threads;

    void LockWorkToBeDone();
    void UnlockWorkToBeDone();
public:
    ThreadPoolManager(unsigned int numberofthreads);

    void Construct_Vecs(int size);
    //This is the function where the main thread will keep looping
    int SocketMain(int numberofthreads);

    //will loop through the Readsets to deduce which one has the least amount of sockets
    int SetSizeFinder();

    void InComingConnectionSetManager(SOCKET clientsocket, sockaddr_in Clientaddress);

    void AddThreadToPool(WorkerThread* InThread) { threads.emplace_back(InThread); }

    inline std::vector<Work*>& GetWorkVector() { return WorkToBeDone; }
};


