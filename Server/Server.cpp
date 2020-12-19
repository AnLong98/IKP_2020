#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define MAX_CLIENTS 10
#define MAX_QUEUE 20

using namespace std;
void AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket, fd_set* readfds);
DWORD WINAPI ProcessIncomingFileRequest(LPVOID param);

//Global variables
HANDLE EmptyQueue;						  // Semaphore which indicates how many (if any) empty spaces are available on queue.
HANDLE FullQueue;						  // Semaphore which indicates how many (if any) sockets are enqueued on IR queue.
HANDLE FinishSignal;    				  // Semaphore to signalize threads to abort.
CRITICAL_SECTION QueueAccess;			  // Critical section for queue access

queue<SOCKET> incomingRequestsQueue;	  //Queue on which sockets with incoming messages are stashed.

int  main(void)
{
	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;
	// Socket used for communication with client
	SOCKET acceptedSockets[MAX_CLIENTS];
	// variable used to store function return value
	int iResult;
	int socketsTaken = 0;
	// Buffer used for storing incoming data
	char recvbuf[DEFAULT_BUFLEN];
	fd_set readfds;
	unsigned long mode = 1; //non-blocking mode

	if (InitializeWindowsSockets() == false)
	{
		// we won't log anything since it will be logged
		// by InitializeWindowsSockets() function
		return 1;
	}

	// Prepare address information structures
	addrinfo *resultingAddress = NULL;
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;       // IPv4 address
	hints.ai_socktype = SOCK_STREAM; // Provide reliable data streaming
	hints.ai_protocol = IPPROTO_TCP; // Use TCP protocol
	hints.ai_flags = AI_PASSIVE;     // 

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &resultingAddress);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(AF_INET,      // IPv4 address famly
						  SOCK_STREAM,  // stream socket
						  IPPROTO_TCP); // TCP

	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket - bind port number and local address 
	// to socket
	iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(resultingAddress);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// Since we don't need resultingAddress any more, free it
	freeaddrinfo(resultingAddress);

	// Set listenSocket in listening mode


	iResult = ioctlsocket(listenSocket, FIONBIO, &mode);
	if (iResult != NO_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", iResult);
		return 0;
	}


	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult != NO_ERROR)
	{
		printf("ioctlsocket failed with error: %ld\n", iResult);
		return 0;
	}

	printf("Server initialized, waiting for clients.\n");

	do
	{
		FD_ZERO(&readfds);
		FD_SET(listenSocket, &readfds);
		for (int i = 0; i < socketsTaken; i++)
		{
			FD_SET(acceptedSockets[i], &readfds);
		}

		timeval timeVal;
		timeVal.tv_sec = 3;
		timeVal.tv_usec = 0;

		int result = select(0, &readfds, NULL, NULL, &timeVal); //

		if (result == 0) {
			continue;
		}
		else if (result == SOCKET_ERROR) {

			//TODO: Handle this error

			printf("Errorcina je %d ", WSAGetLastError());
		}
		else {
			if (FD_ISSET(listenSocket, &readfds) && socketsTaken < MAX_CLIENTS)
			{
				AcceptIncomingConnection(acceptedSockets, &socketsTaken, listenSocket, &readfds);
			}

			for (int i = 0; i < socketsTaken; i++)
			{
				if (FD_ISSET(acceptedSockets[i], &readfds))
					incomingRequestsQueue.push(acceptedSockets[i]);
			}

		}


	} while (1);

	for (int i = 0; i < socketsTaken; i++)
	{
		// shutdown the connection since we're done
		iResult = shutdown(acceptedSockets[i], SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSockets[i]);
			WSACleanup();
			return 1;
		}
		closesocket(acceptedSockets[i]);
	}

	// cleanup
	closesocket(listenSocket);
	WSACleanup();

	return 0;
}

bool InitializeWindowsSockets()
{
	WSADATA wsaData;
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return false;
	}
	return true;
}

void AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket, fd_set* readfds)
{
	acceptedSockets[*freeIndex] = accept(listenSocket, NULL, NULL);

	if (acceptedSockets[*freeIndex] == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	unsigned long mode = 1; //non-blocking mode
	int iResult = ioctlsocket(acceptedSockets[*freeIndex], FIONBIO, &mode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);
	(*freeIndex)++;

}

DWORD WINAPI ProcessIncomingFileRequest(LPVOID param)
{
	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { FinishSignal, FullQueue };
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
		
		EnterCriticalSection(&QueueAccess);
		SOCKET requestSocket = incomingRequestsQueue.front();
		incomingRequestsQueue.pop();
		LeaveCriticalSection(&QueueAccess);

		//TODO Implement recv functions and call them here.
	}
	return 0;
}
