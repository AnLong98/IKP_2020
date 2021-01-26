#include "../FTPServerFunctions/ConnectionFuncs.h"
#define SOCKET_BUFFER_SIZE 20

CRITICAL_SECTION ConnectionsAccess;
int isInitConnectionsHandle = 0;

void InitConnectionsHandle()
{
	InitializeCriticalSection(&ConnectionsAccess);
	isInitConnectionsHandle = 1;
}

void DeleteConnectionsHandle()
{
	DeleteCriticalSection(&ConnectionsAccess);
	isInitConnectionsHandle = 0;
}

int ShutdownConnection(SOCKET socket)
{
	// shutdown the connection
	int iResult = shutdown(socket, SD_BOTH);
	if (iResult == SOCKET_ERROR)
	{
		closesocket(socket);
		return -1;
	}
	closesocket(socket);
	return 0;
}

SOCKET* GetAllSockets(LinkedList<SOCKET>* socketsArray, int* socketsCount)
{
	if (!isInitConnectionsHandle)
		return NULL;

	
	EnterCriticalSection(&ConnectionsAccess);
	if (socketsArray->Count() == 0)
	{
		LeaveCriticalSection(&ConnectionsAccess);
		
		*socketsCount = 0;
		return NULL;

	}

	SOCKET* array = (SOCKET*)malloc(socketsArray->Count() * sizeof(SOCKET));
	if (array == NULL)
		return NULL;
	int i = 0;
	ListNode<SOCKET>* nodeFront = socketsArray->AcquireIteratorNodeFront();
	while (nodeFront != nullptr)
	{
		array[i] = nodeFront->GetValue();
		i++;

		nodeFront = nodeFront->Next();
	}

	LeaveCriticalSection(&ConnectionsAccess);
	
	*socketsCount = i;
	return array;

}


int AcceptIncomingConnection(LinkedList<SOCKET>* socketsList, SOCKET listenSocket)
{
	if (!isInitConnectionsHandle)
		return -2;

	SOCKET socket = accept(listenSocket, NULL, NULL);

	if (socket == INVALID_SOCKET)
	{
		closesocket(listenSocket);
		return -1;
	}

	unsigned long mode = 1; //non-blocking mode
	int iResult = ioctlsocket(socket, FIONBIO, &mode);
	
	EnterCriticalSection(&ConnectionsAccess);
	socketsList->PushFront(socket);
	LeaveCriticalSection(&ConnectionsAccess);
	

	return 0;
}

int RemoveSocketFromArray(LinkedList<SOCKET>* socketsList, SOCKET socket)
{
	if (!isInitConnectionsHandle)
		return -2;

	
	EnterCriticalSection(&ConnectionsAccess);
	ListNode<SOCKET>* nodeFront = socketsList->AcquireIteratorNodeFront();

	while (nodeFront != nullptr)
	{
		if (nodeFront->GetValue() == socket)
		{
			socketsList->RemoveElement(nodeFront);
			LeaveCriticalSection(&ConnectionsAccess);
			return 0;
		}

		nodeFront = nodeFront->Next();
	}

	LeaveCriticalSection(&ConnectionsAccess);
	return -1;
}

int CheckSetSockets(LinkedList<SOCKET>* socketsList, fd_set* readfds, Queue<SOCKET>* communicationQueue, HashMap<SOCKET>* processingSocketsMap)
{
	if (!isInitConnectionsHandle)
		return -2;

	int setSocketsCount = 0;
	
	EnterCriticalSection(&ConnectionsAccess);
	ListNode<SOCKET>* nodeFront = socketsList->AcquireIteratorNodeFront();
	while (nodeFront != nullptr)
	{
		char socketBuffer[SOCKET_BUFFER_SIZE];
		SOCKET socket = nodeFront->GetValue();
		sprintf_s(socketBuffer, "%d", socket);
		if ( processingSocketsMap->DoesKeyExist((const char*) socketBuffer))
		{
			nodeFront = nodeFront->Next();
			continue;  //check if socket is already under processing
		}
		if (FD_ISSET(socket, readfds))
		{
			if (communicationQueue->isFull())
			{
				printf("\nQUEUE FULL");
				LeaveCriticalSection(&ConnectionsAccess);
				return setSocketsCount;
			}
				
			communicationQueue->Enqueue(socket);
			//Add socket to processing map to avoid double reading
			processingSocketsMap->Insert((const char*)socketBuffer, socket);
			setSocketsCount++;
		}
		nodeFront = nodeFront->Next();
	}
	LeaveCriticalSection(&ConnectionsAccess);
	return setSocketsCount;
}

int DisconnectBrokenSockets(LinkedList<SOCKET>* socketsList)
{
	if (!isInitConnectionsHandle)
		return -2;

	int brokenSockets = 0;

	EnterCriticalSection(&ConnectionsAccess);
	ListNode<SOCKET>* nodeFront = socketsList->AcquireIteratorNodeFront();
	while (nodeFront != nullptr)
	{
		SOCKET socket = nodeFront->GetValue();
		if (IsSocketBroken(socket))
		{
			ShutdownConnection(socket);
			ListNode<SOCKET>* nodeRemove = nodeFront;
			nodeFront = nodeFront->Next();
			socketsList->RemoveElement(nodeRemove);
			brokenSockets++;
			continue;
		}
		nodeFront = nodeFront->Next();
	}

	LeaveCriticalSection(&ConnectionsAccess);;
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