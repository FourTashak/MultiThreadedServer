# MultiThreadedServer

The Startup function gets called and resized the vectors according to the amount of threads. The threads are then created and set to run in a loop waiting for client sockets
to receive, process and send data to. The main thread then indefinitely loops accepting connections and passing the connections on to the worker threads. I programmed it
evenly distribute connections between threads so that one thread is not overloaded with work while the other threads wait, the function that does the distributing is called
SetManager. To shutdown the server the user has to type "Shutdown" into the console, this will set a flag for the worker threads and wait for them to join, when the worker
threads see the flag is set they will end all connections and break out of the loop resulting in joining the main thread. The main thread will then start cleaning up.
