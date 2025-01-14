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

class threadPool
{
    class Connections;
    class Threads;
private:
    std::vector<fd_set> Readsets;
    std::vector<std::vector<Connections>> Cons;

    std::vector<Work*> WorkToBeDone;

    std::chrono::steady_clock::time_point LastTime;
public:
    threadPool(unsigned int numberofthreads)
    {
        Cons.resize(numberofthreads);
        Readsets.reserve(numberofthreads);
        Construct_Vecs(numberofthreads);
        Threads th(numberofthreads,Cons,Readsets,this);
        SocketMain(th);
    }
    void Construct_Vecs(int size);
    //This is the function where the main thread will keep looping
    int SocketMain(Threads& Th);

    //will loop through the Readsets to deduce which one has the least amount of sockets
    int SetSizeFinder();

    void InComingConnectionSetManager(SOCKET clientsocket, sockaddr_in Clientaddress);

    class Threads
    {
        friend threadPool;
    public:
        Threads(unsigned int numberofthreads, std::vector<std::vector<Connections>>& ConVec, std::vector<fd_set>& ReadVec,threadPool* MainPoolPtr)
        {
            OwnerPool = MainPoolPtr;
            JoinFlag.resize(numberofthreads);
            for (unsigned int i = 0; i < numberofthreads; i++)
            {
                //Worker threads get emplaced back into the vector and start running the run function
                threads.emplace_back(std::thread(&Threads::run, this, i, std::ref(ConVec), std::ref(ReadVec), std::ref(JoinFlag)));
            }
        }
        //This function is used when authentication is successful
        //it send stock name and prices, and how many stocks the customer has and balance

        int SendCusInfo(SOCKET clientsocket, std::string Cus);

        //this is the main function for the worker threads, here the worker threads are in an infinite loop where they check on sockets assigned to them
        //and receive data if there is data to be received.
        void run(int number, std::vector<std::vector<Connections>>& ConVec, std::vector<fd_set>& ReadVec, std::vector<bool>& Joinflag);

    private:
        void GetWorkAndSendResult(Work* InWork);

        std::vector<std::thread> threads;
        std::vector<bool> JoinFlag;

        threadPool* OwnerPool;
    };

    class Connections
    {
        friend Threads;
        friend threadPool;
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
    private:
        std::shared_ptr<SOCKET> sock_;
        sockaddr_in Clientaddress_;
        std::string Name;
    };
};


