#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>
#include <list>
#include <unordered_map>
#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"
#include "../FTPServerFunctions/Server_Structs.h"
#include "../PeerToPeerFileTransferFunctions/P2PLimitations.h"
#include "../FTPServerFunctions/ServerFuncs.h"
#include "../FileIO_Functions/FileIO.h"
#define DEFAULT_PORT "27016"
#define MAX_QUEUE 20
#define SERVER_THREADS 6
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

using namespace std;
bool InitializeWindowsSockets();
int CheckSetSockets(int* socketsTaken, SOCKET acceptedSockets[], fd_set* readfds);
int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART** unallocatedPartsArray);
int PackExistingFileResponse(FILE_RESPONSE* response, FILE_DATA fileData, FILE_REQUEST request, int* serverOwnedParts);
int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName);
int AddClientInfo(SOCKET* socket, FILE_DATA data, SOCKADDR_IN clientInfo);
int RemoveClientInfo(SOCKET* clientSocket);
DWORD WINAPI ProcessIncomingFileRequest(LPVOID param);

//Global variables
HANDLE EmptyQueue;						  // Semaphore which indicates how many (if any) empty spaces are available on queue.
HANDLE FullQueue;						  // Semaphore which indicates how many (if any) sockets are enqueued on IR queue.
HANDLE FinishSignal;    				  // Semaphore to signalize threads to abort.
CRITICAL_SECTION QueueAccess;			  // Critical section for queue access
CRITICAL_SECTION FileMapAccess;			  // Critical section for file map access
CRITICAL_SECTION ClientListAccess;			  // Critical section for clients list access
CRITICAL_SECTION AcceptedSocketsAccess;			  // Critical section for accepted sockets access
CRITICAL_SECTION ProcessingSocketsAccess;			  // Critical section for processing sockets access

//Data structures
queue<SOCKET*> incomingRequestsQueue;	  //Queue on which sockets with incoming messages are stashed.
unordered_map<string, FILE_DATA> fileInfoMap;
list<CLIENT_INFO> clientInformationsList;
SOCKET acceptedSockets[MAX_CLIENTS];
int processingSockets[MAX_CLIENTS];
int socketsTaken = 0;

int  main(void)
{
	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;
	// Socket used for communication with client
	
	// variable used to store function return value
	int iResult;
	// Buffer used for storing incoming data
	fd_set readfds;
	unsigned long mode = 1; //non-blocking mode

	HANDLE processors[SERVER_THREADS] = { NULL };
	DWORD processorIDs[SERVER_THREADS];


	//Create Semaphores
	EmptyQueue = CreateSemaphore(0, SERVER_THREADS, SERVER_THREADS, NULL);
	FullQueue = CreateSemaphore(0, 0, SERVER_THREADS, NULL);
	FinishSignal = CreateSemaphore(0, 0, SERVER_THREADS, NULL);

	//Init critical sections and threads if semaphores are ok
	if (EmptyQueue  && FullQueue && FinishSignal)
	{
		InitializeCriticalSection(&QueueAccess);
		InitializeCriticalSection(&FileMapAccess);
		InitializeCriticalSection(&ClientListAccess);
		InitializeCriticalSection(&AcceptedSocketsAccess);
		InitializeCriticalSection(&ProcessingSocketsAccess);

		for(int i = 0; i < SERVER_THREADS; i++)
			processors[i] = CreateThread(NULL, 0, &ProcessIncomingFileRequest, (LPVOID)0, 0, processorIDs + i);

		for (int i = 0; i < SERVER_THREADS; i++)
		{
			if (!processors[i]) {

				ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
				printf("\nReleased finish");

				for(int i = 0; i < SERVER_THREADS; i++)
					SAFE_DELETE_HANDLE(processors[i]);
				SAFE_DELETE_HANDLE(EmptyQueue);
				SAFE_DELETE_HANDLE(FullQueue);
				SAFE_DELETE_HANDLE(FinishSignal);

				DeleteCriticalSection(&QueueAccess);
				DeleteCriticalSection(&FileMapAccess);
				DeleteCriticalSection(&ClientListAccess);
				DeleteCriticalSection(&AcceptedSocketsAccess);
				DeleteCriticalSection(&ProcessingSocketsAccess);
				return 0;
			}
		}
	}

	

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

	while(true)
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

			//TODO: Handle this error, check which socket cause error and see if it is in sockets to close list
			

			printf("Errorcina je %d ", WSAGetLastError());
			ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
			break;
		}
		else {
			if (FD_ISSET(listenSocket, &readfds) && socketsTaken < MAX_CLIENTS)
			{
				EnterCriticalSection(&AcceptedSocketsAccess);
				AcceptIncomingConnection(acceptedSockets, &socketsTaken, listenSocket);
				printf("Primio konekciju na %d", socketsTaken - 1);
				LeaveCriticalSection(&AcceptedSocketsAccess);
			}

			
			EnterCriticalSection(&AcceptedSocketsAccess);
			CheckSetSockets(&socketsTaken, acceptedSockets, &readfds);
			LeaveCriticalSection(&AcceptedSocketsAccess);
			

		}


	}

	for (int i = 0; i < SERVER_THREADS; i++)
		SAFE_DELETE_HANDLE(processors[i]);
	SAFE_DELETE_HANDLE(EmptyQueue);
	SAFE_DELETE_HANDLE(FullQueue);
	SAFE_DELETE_HANDLE(FinishSignal);

	DeleteCriticalSection(&QueueAccess);
	DeleteCriticalSection(&FileMapAccess);
	DeleteCriticalSection(&ClientListAccess);
	DeleteCriticalSection(&AcceptedSocketsAccess);
	DeleteCriticalSection(&ProcessingSocketsAccess);

	for (int i = 0; i < socketsTaken; i++)
	{
		ShutdownConnection(acceptedSockets + i);
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


DWORD WINAPI ProcessIncomingFileRequest(LPVOID param)
{
	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { FinishSignal, FullQueue };
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
		printf("Taken full queue");
		EnterCriticalSection(&QueueAccess);
		SOCKET* requestSocket = incomingRequestsQueue.front();  //Get request from queue
		incomingRequestsQueue.pop();
		LeaveCriticalSection(&QueueAccess);

		FILE_DATA fileData;
		FILE_PART* fileParts = NULL;
		FILE_REQUEST fileRequest;
		FILE_RESPONSE fileResponse;
		int serverOwnedParts[FILE_PARTS];
		int isAssignedWithPart = 0;

		//Receive request from socket
		int result = RecvFileRequest(*requestSocket, &fileRequest);
		if (result == -1)
		{
			int processingSocketIndex = requestSocket - acceptedSockets;
			printf("\nDiskonektovao se %d", processingSocketIndex);
			RemoveClientInfo(requestSocket);
			EnterCriticalSection(&AcceptedSocketsAccess);
			ShutdownConnection(requestSocket);
			RemoveSocketFromArray(acceptedSockets,requestSocket, &socketsTaken );
			
			EnterCriticalSection(&ProcessingSocketsAccess);
			for (int i = processingSocketIndex; i < MAX_CLIENTS - 1; i++)
			{
				processingSockets[i] = processingSockets[i + 1];
			}
			processingSockets[MAX_CLIENTS - 1] = 0;
			LeaveCriticalSection(&ProcessingSocketsAccess);
			LeaveCriticalSection(&AcceptedSocketsAccess);
			ReleaseSemaphore(EmptyQueue, 1, NULL);
			printf("\nReleased empty queue");
			continue;
			
		}

		//Remove socket index from processing array
		EnterCriticalSection(&ProcessingSocketsAccess);
		processingSockets[requestSocket - acceptedSockets] = 0;
		LeaveCriticalSection(&ProcessingSocketsAccess);
		
		size_t sizeFound = fileInfoMap.count(fileRequest.fileName);
		//LeaveCriticalSection(&FileMapAccess);

		if ( sizeFound > 0)//File is loaded and can be given back to client
		{
			EnterCriticalSection(&FileMapAccess);
			fileData = fileInfoMap.find(fileRequest.fileName)->second;
			LeaveCriticalSection(&FileMapAccess);
			PackExistingFileResponse(&fileResponse, fileData, fileRequest, serverOwnedParts);
			fileParts = fileData.filePartDataArray;
			isAssignedWithPart = 1;
			
		}
		else // We need to load the file first
		{
			char* fileBuffer = NULL;
			size_t fileSize;
			int result = ReadFileIntoMemory(fileRequest.fileName, &fileBuffer, &fileSize);

			if (result != 0)//File probably doesn't exist on server
			{
				fileResponse.fileExists = 0;
				fileResponse.clientPartsNumber = 0;
				fileResponse.filePartToStore = 0;
				fileResponse.serverPartsNumber = 0;
				isAssignedWithPart = -1;
			}
			else //File is on server and loaded into buffer
			{
				DivideFileIntoParts(fileBuffer, fileSize, FILE_PARTS, &fileParts);
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
				fileData.nextPartToAssign = 0;
				memcpy(fileData.fileName, fileRequest.fileName, MAX_FILE_NAME);
				fileData.partsOnClients = 0;

				//Add new file data structure to map, no need for CS because no one owns this structure yet
				fileInfoMap[fileRequest.fileName] = fileData;
			}

		}

		if (SendFileResponse(*requestSocket, fileResponse) != 0)
		{
			RemoveClientInfo(requestSocket);
			EnterCriticalSection(&AcceptedSocketsAccess);
			ShutdownConnection(requestSocket);
			RemoveSocketFromArray(acceptedSockets, requestSocket, &socketsTaken);
			LeaveCriticalSection(&AcceptedSocketsAccess);
			ReleaseSemaphore(EmptyQueue, 1, NULL);
			continue;
		}

		int sockERR = 0;
		for (int i = 0; i < fileResponse.serverPartsNumber; i++)
		{
			int partIndex = serverOwnedParts[i];
			FILE_PART partToSend = fileParts[partIndex];
			if (SendFilePart(*requestSocket, partToSend.partStartPointer, partToSend.partSize, partIndex) != 0)
			{
				RemoveClientInfo(requestSocket);
				EnterCriticalSection(&AcceptedSocketsAccess);
				ShutdownConnection(requestSocket);
				RemoveSocketFromArray(acceptedSockets, requestSocket, &socketsTaken);
				LeaveCriticalSection(&AcceptedSocketsAccess);
				sockERR = 1;
				break;
			}
		}
		if (sockERR == 1) {
			ReleaseSemaphore(EmptyQueue, 1, NULL);
			printf("\nReleased empty queue");
			continue; //Skip processing as socket was compromised
		}
		if (isAssignedWithPart == 0)
			AssignFilePartToClient(fileRequest.requesterListenAddress, fileRequest.fileName);
		AddClientInfo(requestSocket, fileData, fileRequest.requesterListenAddress);
		ReleaseSemaphore(EmptyQueue, 1, NULL);
		printf("\nReleased empty queue");
	}
	return 0;
}


int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART** unallocatedPartsArray)
{
	size_t partSize = fileSize / parts;
	size_t totalSizeAccounted = 0;

	*unallocatedPartsArray = (FILE_PART*)malloc(sizeof(FILE_PART) * parts * 2); //Allocate double the size

	if (*unallocatedPartsArray == NULL)
	{
		ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
		printf("\nReleased finish");
		return -1;
	}
	int partNumber = 0;
	for (int i = 0; i < parts; i++)
	{
		if (i == parts - 1)
		{
			(*unallocatedPartsArray)[i].partSize = fileSize - totalSizeAccounted;
		}
		else
		{
			(*unallocatedPartsArray)[i].partSize = partSize;
		}
		(*unallocatedPartsArray)[i].filePartNumber = partNumber++;
		(*unallocatedPartsArray)[i].isServerOnly = 1; //Assign part to server first
		(*unallocatedPartsArray)[i].partStartPointer = loadedFileBuffer + i * partSize;
		totalSizeAccounted += partSize;
	}

	for (int i = parts; i < 2 * parts; i++)
	{
		(*unallocatedPartsArray)[i].isServerOnly = 1;
	}

	return 0;
}

int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName)
{
	int partAssigned = 0;

	EnterCriticalSection(&FileMapAccess);
	FILE_DATA fileData = fileInfoMap[fileName];
	if (fileData.partArraySize == fileData.partsOnClients)//Parts array is full and should be increased
	{
		fileData.filePartDataArray = (FILE_PART*)realloc(fileData.filePartDataArray, fileData.partArraySize + FILE_PARTS); //Increase array by the count of file parts
		fileData.partArraySize += FILE_PARTS;
	}
	if (fileData.filePartDataArray == NULL)
	{
		LeaveCriticalSection(&FileMapAccess);
		ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
		printf("\nReleased finish");
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
	memcpy(fileData.fileName, fileName, strlen(fileName));														//Copy file name

	fileInfoMap[fileName] = fileData;																			//Save changes
	LeaveCriticalSection(&FileMapAccess);

	return partAssigned;
}


int UnassignFilePart(SOCKADDR_IN clientInfo, char* fileName)
{
	EnterCriticalSection(&FileMapAccess);
	int clientPartIndex = -1;
	FILE_DATA fileData = fileInfoMap[fileName];
	//Find index of user assigned part
	for (int i = 0; i < fileData.partsOnClients; i++)
	{
		if (fileData.filePartDataArray[i].isServerOnly)continue; //Skip server owned parts
		unsigned short filePartPort = fileData.filePartDataArray[i].clientOwnerAddress.sin_port;
		char *filePartIp = inet_ntoa(fileData.filePartDataArray[i].clientOwnerAddress.sin_addr);
		if (filePartPort == clientInfo.sin_port && strcmp(filePartIp, inet_ntoa(clientInfo.sin_addr)) == 0)
		{
			clientPartIndex = i;
			break;
		}
	}
	int filePartToShift = fileData.filePartDataArray[clientPartIndex].filePartNumber;
	if (clientPartIndex == -1)
	{
		LeaveCriticalSection(&FileMapAccess);
		return -1;
	}

	
	int isShifted = 0;
	//Check if any other user has this file part and put his data here
	for (int i = clientPartIndex + 1; i < fileData.partsOnClients; i++)
	{
		if (fileData.filePartDataArray[i].isServerOnly)continue;
		if (fileData.filePartDataArray[i].filePartNumber == filePartToShift)
		{
			fileData.filePartDataArray[clientPartIndex].clientOwnerAddress = fileData.filePartDataArray[i].clientOwnerAddress;
			clientPartIndex = i;
			isShifted = 1;

		}
	}

	if (!isShifted)
	{
		fileData.filePartDataArray[filePartToShift].isServerOnly = 1;
	}
	
	//Shift back only if client's part is not among first 10 parts
	if (clientPartIndex >= FILE_PARTS)
	{
		//Shift remaining file parts back one place to fill the gap
		for (int i = clientPartIndex + 1; i < fileData.partsOnClients; i++)
		{
			fileData.filePartDataArray[i - 1] = fileData.filePartDataArray[i];
		}
	}

	fileData.partsOnClients--; //There is one less client owned part now.
	fileInfoMap[fileName] = fileData;
	LeaveCriticalSection(&FileMapAccess);
	return 0;
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
			partInfo.clientOwnerAddress = fileData.filePartDataArray[i].clientOwnerAddress;
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


int CheckSetSockets(int* socketsTaken, SOCKET acceptedSockets[], fd_set* readfds )
{
	for (int i = 0; i < *socketsTaken; i++)
	{
		EnterCriticalSection(&ProcessingSocketsAccess);
		if (processingSockets[i] == 1)
		{
			LeaveCriticalSection(&ProcessingSocketsAccess);
			continue;  //check if socket is already under processing
		}
		LeaveCriticalSection(&ProcessingSocketsAccess);

		if (FD_ISSET(acceptedSockets[i], readfds))
		{
			const int semaphoreNum = 2;
			HANDLE semaphores[semaphoreNum] = { FinishSignal, EmptyQueue };
			//Ovde se zakuca ako iskljucim par klijenata
			DWORD waitResult = WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE);
			
			if(waitResult == WAIT_OBJECT_0 + 1)
			{
				printf("\nTaken Empty queue");
				EnterCriticalSection(&QueueAccess);
				printf("\nSoket %d primio", i);
				incomingRequestsQueue.push(acceptedSockets + i);
				LeaveCriticalSection(&QueueAccess);
				ReleaseSemaphore(FullQueue, 1, NULL);
				printf("\nReleased full queue");
				//Add socket index to processing list to avoid double reading
				EnterCriticalSection(&ProcessingSocketsAccess);
				processingSockets[i] = 1;
				LeaveCriticalSection(&ProcessingSocketsAccess);
					
			}
			else
			{
				return -1;
			}
		}

	}

	return 0;
}


int AddClientInfo(SOCKET* socket, FILE_DATA data, SOCKADDR_IN clientAddress)
{
	//TODO ADD user's socket info 

	EnterCriticalSection(&ClientListAccess);
	list<CLIENT_INFO>::iterator it;
	for (it = clientInformationsList.begin(); it != clientInformationsList.end(); ++it) {
		if (it->clientSocket == socket) //Client's socket already exists here so we will just append new data
		{
			if (it->fileDataArraySize == it->ownedFilesCount)
			{
				it->clientOwnedFiles = (FILE_DATA*)realloc(it->clientOwnedFiles, it->fileDataArraySize  + FILE_PARTS);
				it->fileDataArraySize *= 2;
			}
			it->clientOwnedFiles[it->ownedFilesCount] = data;
			it->ownedFilesCount++;
			LeaveCriticalSection(&ClientListAccess);
			return 0;
		}
	}

	CLIENT_INFO info;
	info.clientOwnedFiles = (FILE_DATA*)malloc(FILE_PARTS * sizeof(FILE_DATA));
	info.clientAddress = clientAddress;
	info.clientSocket = socket;
	info.fileDataArraySize = FILE_PARTS;
	info.ownedFilesCount = 1;
	info.clientOwnedFiles[0] = data;
	clientInformationsList.push_front(info);
	LeaveCriticalSection(&ClientListAccess);
	return 0;

}


int RemoveClientInfo(SOCKET* clientSocket)
{
	EnterCriticalSection(&ClientListAccess);
	list<CLIENT_INFO>::iterator it;
	for (it = clientInformationsList.begin(); it != clientInformationsList.end(); ++it) {
		if (it->clientSocket == clientSocket)
		{
			for (int i = 0; i < it->ownedFilesCount; i++)
			{
				UnassignFilePart(it->clientAddress, it->clientOwnedFiles[i].fileName);

			}
			free(it->clientOwnedFiles);
			clientInformationsList.erase(it);
			LeaveCriticalSection(&ClientListAccess);
			return 0;
		}
	}

	LeaveCriticalSection(&ClientListAccess);
	return -1;
}


