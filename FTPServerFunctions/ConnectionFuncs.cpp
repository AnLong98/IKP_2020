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