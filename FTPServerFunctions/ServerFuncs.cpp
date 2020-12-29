#include "../FTPServerFunctions/ServerFuncs.h"

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

int RemoveSocketFromArray(SOCKET acceptedSockets[], int *freeIndex)
{
	//IMPLEMENT THIS
	return 0;
}