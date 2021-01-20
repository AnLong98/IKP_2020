#include "../FTPServerFunctions/ConnectionFuncs.h"

int ShutdownConnection(SOCKET* socket)
{
	// shutdown the connection
	int iResult = shutdown(*socket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		closesocket(*socket);
		return -1;
	}
	closesocket(*socket);
	return 0;
}


int AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket)
{
	acceptedSockets[*freeIndex] = accept(listenSocket, NULL, NULL);

	if (acceptedSockets[*freeIndex] == INVALID_SOCKET)
	{
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}

	unsigned long mode = 1; //non-blocking mode
	int iResult = ioctlsocket(acceptedSockets[*freeIndex], FIONBIO, &mode);
	(*freeIndex)++;
	return 0;
}

int RemoveSocketFromArray(SOCKET acceptedSockets[], SOCKET* socket, int *freeIndex)
{
	for (int i = 0; i < *freeIndex; i++)
	{
		if (acceptedSockets + i == socket)
		{
			for (int j = i + 1; j < *freeIndex; j++)
			{
				acceptedSockets[j - 1] = acceptedSockets[j];
			}
			acceptedSockets[(*freeIndex) - 1] = INVALID_SOCKET;
			(*freeIndex)--;
			return 0;
		}
	}
	return -1;
}

int CheckSetSockets(int* socketsTaken, SOCKET acceptedSockets[], fd_set* readfds, Queue<SOCKET*>* communicationQueue, HashMap<SOCKET*>* processingSocketsMap)
{
	int setSocketsCount = 0;
	for (int i = 0; i < *socketsTaken; i++)
	{
		if (processingSocketsMap->DoesKeyExist((char*)(acceptedSockets + i)))
		{
			continue;  //check if socket is already under processing
		}
		if (FD_ISSET(acceptedSockets[i], readfds))
		{
			if (communicationQueue->isFull())
				return setSocketsCount;
			printf("\nSoket %d primio", i);
			communicationQueue->Enqueue(acceptedSockets + i);
			//Add socket to processing map to avoid double reading
			processingSocketsMap->Insert((const char*)(acceptedSockets + i), acceptedSockets + i);
			setSocketsCount++;
		}

	}

	return setSocketsCount;
}

int DisconnectBrokenSockets(SOCKET sockets[], int* socketsCount)
{

	int brokenSockets = 0;
	for (int i = 0; i < *socketsCount; i++)
	{
		if (IsSocketBroken(sockets[i]))
		{
			ShutdownConnection(sockets + i);
			RemoveSocketFromArray(sockets, sockets + i, socketsCount);
			brokenSockets++;
		}
	}

	return brokenSockets;
}


int IsSocketBroken(SOCKET s)
{
	char buf;
	int err = recv(s, &buf, 1, MSG_PEEK);
	if (err == SOCKET_ERROR)
	{
		if (WSAGetLastError() != WSAEWOULDBLOCK)
		{
			return 1;
		}
	}
	return 0;
}