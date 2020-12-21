#pragma once
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <unordered_map>
#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"
#include "../FTPServerFunctions/Server_Structs.h"
#include "../FileIO_Functions/FileIO.h"

#define DEFAULT_PORT "27016"
#define MAX_CLIENTS 10
#define MAX_QUEUE 20

using namespace std;
bool InitializeWindowsSockets();
void AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket, fd_set* readfds);
int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART* unallocatedPartsArray);
int PackExistingFileResponse(FILE_RESPONSE* response, FILE_DATA fileData, FILE_REQUEST request, int* serverOwnedParts);
int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName);
DWORD WINAPI ProcessIncomingFileRequest(LPVOID param);

//Global variables
HANDLE EmptyQueue;						  // Semaphore which indicates how many (if any) empty spaces are available on queue.
HANDLE FullQueue;						  // Semaphore which indicates how many (if any) sockets are enqueued on IR queue.
HANDLE FinishSignal;    				  // Semaphore to signalize threads to abort.
CRITICAL_SECTION QueueAccess;			  // Critical section for queue access
CRITICAL_SECTION FileMapAccess;			  // Critical section for file map access

//Data structures
queue<SOCKET> incomingRequestsQueue;	  //Queue on which sockets with incoming messages are stashed.
unordered_map<string, FILE_DATA> fileInfoMap;

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
		SOCKET requestSocket = incomingRequestsQueue.front();  //Get request from queue
		incomingRequestsQueue.pop();
		LeaveCriticalSection(&QueueAccess);

		FILE_DATA fileData;
		FILE_PART* fileParts = NULL;
		FILE_REQUEST fileRequest;
		FILE_RESPONSE fileResponse;
		int serverOwnedParts[FILE_PARTS];

		//Receive request from socket
		int result = RecvFileRequest(requestSocket, &fileRequest);
		if (result == -1)
		{
			//The was an error and handle it
		}

		EnterCriticalSection(&FileMapAccess);
		size_t sizeFound = fileInfoMap.count(fileRequest.fileName);
		LeaveCriticalSection(&FileMapAccess);

		if ( sizeFound > 0)//File is loaded and can be given back to client
		{
			EnterCriticalSection(&FileMapAccess);
			fileData = fileInfoMap.find(fileRequest.fileName)->second;
			LeaveCriticalSection(&FileMapAccess);
			PackExistingFileResponse(&fileResponse, fileData, fileRequest, serverOwnedParts);
			
		}
		else // We need to load the file first
		{
			char* fileBuffer = NULL;
			size_t fileSize;
			int result = ReadFileIntoMemory(fileRequest.fileName, fileBuffer, &fileSize);

			if (result != 0)//File probably doesn't exist on server
			{
				fileResponse.fileExists = 0;
				fileResponse.clientPartsNumber = 0;
				fileResponse.filePartToStore = 0;
				fileResponse.serverPartsNumber = 0;
			}
			else //File is on server and loaded into buffer
			{
				DivideFileIntoParts(fileBuffer, fileSize, FILE_PARTS, fileParts);
				fileResponse.fileExists = 1;
				fileResponse.clientPartsNumber = 0;
				fileResponse.filePartToStore = 0;
				fileResponse.serverPartsNumber = FILE_PARTS;

				//Add server owned part indexes
				for (int i = 0; i < FILE_PARTS; i++)serverOwnedParts[i] = i;

				//Init new File data structure for newly loaded file
				fileData.filePointer = fileBuffer;
				fileData.filePartDataArray = fileParts;
				fileData.partArraySize = 2 * FILE_PARTS;
				fileData.partsOnClients = 0;
			}

			//Add new file data structure to map, no need for CS because no one owns this structure yet
			fileInfoMap[fileRequest.fileName] = fileData;
			AssignFilePartToClient(fileRequest.requesterListenAddress, fileRequest.fileName);
		}

		SendFileResponse(requestSocket, fileResponse);

		for (int i = 0; i < fileResponse.serverPartsNumber; i++)
		{
			int partIndex = serverOwnedParts[i];
			FILE_PART partToSend = fileParts[partIndex];
			if (SendFilePart(requestSocket, partToSend.partStartPointer, partToSend.partSize, partIndex) != 0)
			{
				//HANDLE SEND ERROR HERE
			}
		}

		ReleaseSemaphore(&EmptyQueue, 1, NULL);
	}
	return 0;
}


int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART* unallocatedPartsArray)
{
	size_t partSize = fileSize / parts;
	size_t totalSizeAccounted = 0;

	unallocatedPartsArray = (FILE_PART*)malloc(sizeof(FILE_PART) * parts * 2); //Allocate double the size

	if (unallocatedPartsArray == NULL)
	{
		//TODO: handle out of memory
		return -1;
	}

	for (int i = 0; i < parts; i++)
	{
		if (i == parts - 1)
		{
			unallocatedPartsArray[i].partSize = fileSize - totalSizeAccounted;
		}
		else
		{
			unallocatedPartsArray[i].partSize = partSize;
		}
		unallocatedPartsArray[i].filePartNumber = 0;
		unallocatedPartsArray[i].isServerOnly = 1; //Assign part to server first
		unallocatedPartsArray[i].partSize = partSize;
		unallocatedPartsArray[i].partStartPointer = loadedFileBuffer + i * partSize;
		totalSizeAccounted += partSize;
	}

	for (int i = parts; i < 2 * parts; i++)
	{
		unallocatedPartsArray[i].isServerOnly = 1;
	}

	return 0;
}

int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName)
{
	int partAssigned = 0;

	EnterCriticalSection(&FileMapAccess);
	FILE_DATA fileData = fileInfoMap[fileName];
	if (fileData.partArraySize == fileData.partArraySize)//Parts array is full and should be increased
	{
		fileData.filePartDataArray = (FILE_PART*)realloc(fileData.filePartDataArray, fileData.partArraySize + FILE_PARTS); //Increase array by the count of file parts
		fileData.partArraySize += FILE_PARTS;
	}
	if (fileData.filePartDataArray == NULL)
	{
		//TODO Handle out of memory
		return -1;
	}
	partAssigned = fileData.nextPartToAssign;
	FILE_PART filePartToAssign = fileData.filePartDataArray[fileData.nextPartToAssign];							//Get data from first 10 assigned file parts
	fileData.filePartDataArray[fileData.partsOnClients].clientOwnerAddress = clientInfo;						//Assign new client's connection info
	fileData.filePartDataArray[fileData.partsOnClients].isServerOnly = 0;										//Mark as client owned
	fileData.filePartDataArray[fileData.partsOnClients].partSize = filePartToAssign.partSize;					//Copy part size
	fileData.filePartDataArray[fileData.partsOnClients].partStartPointer = filePartToAssign.partStartPointer;	//Copy buffer pointer
	fileData.filePartDataArray[fileData.partsOnClients].filePartNumber = filePartToAssign.filePartNumber;		//Copy part number
	fileData.partsOnClients++;																					//Increase number of parts owned by clients
	fileData.nextPartToAssign = (fileData.nextPartToAssign + 1) % FILE_PARTS;									//Set next part to assign

	fileInfoMap[fileName] = fileData;																			//Save changes
	LeaveCriticalSection(&FileMapAccess);

	return partAssigned;
}

int PackExistingFileResponse(FILE_RESPONSE* response, FILE_DATA fileData, FILE_REQUEST request, int* serverOwnedParts)
{
	int serverOwnedPartsCount = 0;
	int clientOwnedPartsCount = 0;
	
	for (int i = 0; i < FILE_PARTS; i++)
	{
		if (fileData.filePartDataArray[i].isServerOnly)
		{
			serverOwnedParts[serverOwnedPartsCount++] = i;
		}
		else
		{
			FILE_PART_INFO partInfo;
			partInfo.partNumber = fileData.filePartDataArray[i].filePartNumber;
			partInfo.clientOwnerAddress = request.requesterListenAddress;
			response->clientParts[clientOwnedPartsCount++] = partInfo;
		}	

	}
	response->fileExists = 1;
	response->clientPartsNumber = clientOwnedPartsCount;
	response->serverPartsNumber = serverOwnedPartsCount;

	int assignedPart = AssignFilePartToClient(request.requesterListenAddress, request.fileName);
	if (assignedPart == -1)
	{
		//HANDLE error
		return -1;
	}
	else
	{
		response->filePartToStore = assignedPart;
	}

	return 0;

}
