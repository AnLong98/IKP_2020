#include "../FTPServerFunctions/ServerFuncs.h"
#include <stdio.h>

int ShutdownConnection(SOCKET* socket)
{
	// shutdown the connection
	int iResult = shutdown(*socket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(*socket);
		return -1;
	}
	closesocket(*socket);
	return 0;
}


int AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket, fd_set* readfds)
{
	acceptedSockets[*freeIndex] = accept(listenSocket, NULL, NULL);

	if (acceptedSockets[*freeIndex] == INVALID_SOCKET)
	{
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return -1;
	}

	unsigned long mode = 1; //non-blocking mode
	int iResult = ioctlsocket(acceptedSockets[*freeIndex], FIONBIO, &mode);
	if (iResult != NO_ERROR)
		printf("ioctlsocket failed with error: %ld\n", iResult);
	(*freeIndex)++;
	return 0;
}