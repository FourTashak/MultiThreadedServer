#include "Socket.h"
#include <cmath>

void ThreadPoolManager::LockWorkToBeDone()
{

}

void ThreadPoolManager::UnlockWorkToBeDone()
{

}

ThreadPoolManager::ThreadPoolManager(unsigned int numberofthreads)
{
    LastTime = std::chrono::high_resolution_clock::now();

    Cons.resize(numberofthreads);
    Readsets.reserve(numberofthreads);
    Construct_Vecs(numberofthreads);
    for (int i = 0; i < numberofthreads; i++) 
    {
        WorkObj* newWorkObj = new WorkObj;
        WorkToBeDone.push_back(newWorkObj);
    }
    SocketMain(numberofthreads);
}

void ThreadPoolManager::Construct_Vecs(int size)
{
    fd_set Temp;
    FD_ZERO(&Temp);
    for (int i = 0; i < size; i++)
    {
        Cons[i].reserve(FD_SETSIZE);
        Readsets.push_back(Temp);
    }
}

int ThreadPoolManager::SocketMain(unsigned int numberofthreads)
{
    // Initialize Winsock2
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0)
    {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        return 1;
    }

    // Create a socket
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Error creating socket: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    u_long nonblockingmode = 1;
    iResult = ioctlsocket(serverSocket, FIONBIO, &nonblockingmode);
    if (iResult == SOCKET_ERROR)
    {
        std::cout << "Error setting socket to nonblocking mode: " << WSAGetLastError();
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Bind the socket to a local address and port
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(12345); // choose a port number
    iResult = bind(serverSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress));
    if (iResult == SOCKET_ERROR)
    {
        std::cout << "Error binding socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    // Listen for incoming connections
    iResult = listen(serverSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        std::cout << "Error listening on socket: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Accept incoming connections
    SOCKET clientSocket;
    sockaddr_in clientAddress;
    int clientAddressSize = sizeof(clientAddress);
    std::cout << "waiting for clients" << std::endl;

    fd_set acceptFds;
    FD_ZERO(&acceptFds);
    FD_SET(serverSocket, &acceptFds);
    timeval T;
    T.tv_usec = 100000;
    T.tv_sec = 0;

    for (unsigned int i = 0; i < numberofthreads; i++)
    {
        WorkerThread* NewWorkerThread = new WorkerThread(i,Cons,Readsets,this);

        std::thread* NewThread = new std::thread([i, this, NewWorkerThread]() mutable {NewWorkerThread->run(i,Cons,Readsets); });

        AddThreadToPool(NewWorkerThread);
    }

    //The loop where the main thread will loop indefinitely, accepting connections and passing the connections over to the worker threads
    while (true)
    {
        if (_kbhit()) //The main thread will check if there is any input in the console
        {
            std::string In;
            std::getline(std::cin, In);

            //If the shutdown command is entered into the console, the main thread will request the worker threads to finish up what they and wait for them to join
            if (In == "Shutdown")
            {
                for (int i = 0; i < threads.size(); i++)
                {
                    threads[i]->JoinFlag = true;
                    threads[i]->ResponsibleOfThread->join();
                }
                break;
            }
        }
        FD_ZERO(&acceptFds);
        FD_SET(serverSocket, &acceptFds);

        //select is used to check the server socket if there are any connections waiting to be accepted
        iResult = select(serverSocket + 1, &acceptFds, NULL, NULL, &T);
        if (iResult == SOCKET_ERROR)
        {
            std::cout << "Socket error occured, Error code: " << WSAGetLastError() << std::endl;
        }
        else if (iResult == 0)
        {

        }
        else
        {
            //the incoming connection is accepted here
            clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddress, &clientAddressSize);
            if (clientSocket != INVALID_SOCKET)
            {
                if (ioctlsocket(clientSocket, FIONBIO, &nonblockingmode) != 0)
                {
                    std::cout << "failed to set client socket to nonblocking mode: " << WSAGetLastError() << std::endl;
                    closesocket(clientSocket);
                }
                else
                {
                    //The SetManager functions purpose is to evenly distribute incomming connections to each thread
                    //it will always assign the new connection to the thread that has the least connections to manage, this evens the load on all threads.
                    InComingConnectionSetManager(clientSocket, clientAddress);
                    char addrStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(clientAddress.sin_addr), addrStr, INET_ADDRSTRLEN);
                    std::cout << "Connected: " << addrStr << ":" << clientAddress.sin_port << std::endl;
                }
            }
            else if (clientSocket == INVALID_SOCKET)
            {
                char addrStr[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &(clientAddress.sin_addr), addrStr, INET_ADDRSTRLEN);
                std::cout << "Error accepting connection from: " << addrStr << ":" << clientAddress.sin_port << " Error code: " << WSAGetLastError() << std::endl;
                closesocket(serverSocket);
            }
        }

        auto start_time = std::chrono::high_resolution_clock::now();

        auto ElapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(start_time - LastTime);

        if (ElapsedTime.count() > 500)
        {
            LastTime = start_time;

            for (int i = 0; i<WorkToBeDone.size(); i++)
            {
                while (true)
                {
                    bool Locked = WorkToBeDone[i]->Lock.try_lock();

                    if (Locked)
                    {
                        for (int j = WorkToBeDone[i]->Works.size() - 1; j >= 0; j--)
                        {
                            if (WorkToBeDone[i]->Works[j]->BMarkedForDelete)
                            {
                                delete WorkToBeDone[i]->Works[j];
                                WorkToBeDone[i]->Works.erase(WorkToBeDone[i]->Works.begin() + j);
                            }
                        }

                        WorkToBeDone[i]->Lock.unlock();
                        break;
                    }
                }   
            }
        }
    }
    // Close the socket and clean up
    closesocket(serverSocket);
    WSACleanup();

    return 0;
}

int ThreadPoolManager::SetSizeFinder()
{
    auto min = Readsets[0].fd_count;
    int setindentifier = 0;
    for (int i = 1; i < Readsets.size(); i++)
    {
        if (min > Readsets[i].fd_count)
        {
            min = Readsets[i].fd_count;
            setindentifier = i;
        }
    }
    return setindentifier;
}

void ThreadPoolManager::InComingConnectionSetManager(SOCKET clientsocket, sockaddr_in Clientaddress)
{
    int i = SetSizeFinder();
    FD_SET(clientsocket, &Readsets[i]);
    std::unique_ptr<SOCKET> clientsocketnew(new SOCKET(clientsocket));
    Cons[i].emplace_back(Connections(std::move(clientsocketnew), Clientaddress));
}

WorkerThread::WorkerThread(int threadIndex,std::vector<std::vector<Connections>>& ConVec, std::vector<fd_set>& ReadVec, ThreadPoolManager* MainPoolPtr)
{
    OwnerPool = MainPoolPtr;
}

void WorkerThread::run(int ThreadIndex, std::vector<std::vector<Connections>>& ConVec, std::vector<fd_set>& ReadVec)
{
    timeval t;
    t.tv_usec = 100000;
    t.tv_sec = 0;
    while (true)
    {
        //if the joinflag gets set by the main thread the worker thread will end all client connections and join.
        if (JoinFlag == true)
        {
            for (int i = 0; i < ConVec[ThreadIndex].size(); i++)
            {
                ConVec[ThreadIndex][i].~Connections();
            }
            break;
        }
        int Sel = 0;
        //Sleep(50);
        //Cleans the fd_set
        FD_ZERO(&(ReadVec[ThreadIndex]));
        //sets the fd_set with assigned sockets
        for (int i = 0; i < ConVec[ThreadIndex].size(); i++)
        {
            FD_SET(*ConVec[ThreadIndex][i].sock_.get(), &ReadVec[ThreadIndex]);
        }
        if (ReadVec[ThreadIndex].fd_count > 0)
        {
            //the select function checks if there is data to be received
            Sel = select(FD_SETSIZE, &ReadVec[ThreadIndex], NULL, NULL, &t);
        }
        if (Sel == SOCKET_ERROR)
        {
            std::cout << "Socket Error at thread number : " << ThreadIndex << std::endl;
            for (int i = 0; i < ReadVec[ThreadIndex].fd_count; i++)
            {
                SOCKET* OutSocket = &ReadVec[ThreadIndex].fd_array[i];

                char error_code[1024];
                int error_code_size = sizeof(error_code);
                int ErrorNum = getsockopt(*OutSocket, SOL_SOCKET, SO_ERROR, error_code, &error_code_size);

                if (ErrorNum < 0)
                {
                    FD_CLR(*ConVec[ThreadIndex][i].sock_.get(), &ReadVec[ThreadIndex]);
                    ConVec[ThreadIndex].erase(ConVec[ThreadIndex].begin() + i);
                }
            }

            Sleep(50);
        }
        else if (Sel == 0) { Sleep(50); }
        else
        {
            //loops through every socket and checks which socket has data to be received
            for (int i = 0; i < ReadVec[ThreadIndex].fd_count; i++)
            {
                SOCKET* SokRef = ConVec[ThreadIndex][i].sock_.get();
                fd_set* SetRef = &ReadVec[ThreadIndex];

                if (FD_ISSET(*SokRef, SetRef))
                {
                    char buffer[1024];
                    int bytes_rec = recv(*ConVec[ThreadIndex][i].sock_.get(), buffer, sizeof(buffer), 0);
                    if (bytes_rec == -1) //if theres an error with the socket, the thread will set the customer as not logged in and will remove the socket from the vector containing it
                    {
                        std::cout << "Socket Error, Disconnecting Client" << std::endl;
                        FD_CLR(*ConVec[ThreadIndex][i].sock_.get(), &ReadVec[ThreadIndex]);
                        ConVec[ThreadIndex].erase(ConVec[ThreadIndex].begin() + i);
                    }
                    else if (bytes_rec == 0) // If connection is no longer alive, the thread will set the customer as not logged in and will remove the socket from the vector containing it
                    {
                        std::cout << "Socket Error, Disconnecting Client" << std::endl;
                        FD_CLR(*ConVec[ThreadIndex][i].sock_.get(), &ReadVec[ThreadIndex]);
                        ConVec[ThreadIndex].erase(ConVec[ThreadIndex].begin() + i);
                    }
                    else //If connection is not closed and there is data waiting to be read on socket
                    {
                        std::vector<int> Numbers;
                        std::string Ender = "S";

                        for (char C : buffer)
                        {
                            if (0 > Ender.compare(&C)) break;

                            Numbers.push_back(std::stoi(&C));
                        }

                        Work *NewWork = new Work(Numbers, ThreadIndex, *ConVec[ThreadIndex][i].sock_.get());

                        std::mutex* ThisThreadWorkLock = &OwnerPool->GetWorkVector()[ThreadIndex]->Lock;

                        while (true)
                        {
                            bool CanLock = ThisThreadWorkLock->try_lock();

                            if (CanLock)
                            {
                                OwnerPool->GetWorkVector()[ThreadIndex]->Works.push_back(std::move(NewWork));

                                ThisThreadWorkLock->unlock();

                                break;
                            }
                        }
                    }
                }
            }
        }

        WorkObj* CurrentThreadWork = OwnerPool->GetWorkVector()[ThreadIndex];

        while (true)
        {
            bool CanLock = CurrentThreadWork->Lock.try_lock();

            if (CanLock)
            {
                for (int i = OwnerPool->GetWorkVector()[ThreadIndex]->Works.size() - 1; i >= 0; i--)
                {
                    Work* CurrentWork = OwnerPool->GetWorkVector()[ThreadIndex]->Works[i];

                    int reesult = 1;

                    for (int Num : CurrentWork->Numbers)
                    {
                        reesult *= Num;
                    }

                    std::string StringMessage = std::to_string(reesult);

                    const char* Message = StringMessage.c_str();

                    int result = send(*CurrentWork->QuerierSocket, Message, StringMessage.size(), 0);

                    if (result > 0)
                    {
                        CurrentWork->BMarkedForDelete = true;
                    }
                }

                CurrentThreadWork->Lock.unlock();

                break;
            }
            else
            {
                std::cout << "Debug" << std::endl;
            }
        }
    }
}

void WorkerThread::GetWorkAndSendResult(Work* InWork)
{
    InWork->BTaken.store(true);

    int64_t Result = 1;

    for (int Num : InWork->Numbers)
    {
        Result *= Num;
    }

    InWork->Result = Result;

    std::string Packet = std::to_string(Result);

    int result = send(*InWork->QuerierSocket, Packet.c_str(), Packet.size(), 0);

    InWork->BMarkedForDelete = true;
}
