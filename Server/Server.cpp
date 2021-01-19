#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "../DataStructures/HashMap.h"
#include "../DataStructures/Queue.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"
#include "../PeerToPeerFileTransferFunctions/P2PLimitations.h"
#include "../ServerFunctions/ClientInfoManagement.h"
#include "../ServerFunctions/FilePartsManagement.h"
#include "../FTPServerFunctions/ConnectionFuncs.h"
#include "../ServerFunctions/ServerStructs.h"
#include "../FileIO_Functions/FileIO.h"
#define DEFAULT_PORT "27016"
#define MAX_QUEUE 20
#define SERVER_THREADS 6
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

bool InitializeWindowsSockets();
DWORD WINAPI ProcessIncomingFileRequest(LPVOID param);


int  main(void)
{
	{
		//Server data structures
		HashMap<SOCKET*> processingSocketsMap;
		HashMap<FILE_DATA> fileInfoMap;
		Queue<SOCKET*> incomingRequestsQueue;
		HashMap<CLIENT_INFO> clientInformationsMap;
		SOCKET acceptedSockets[MAX_CLIENTS];
		HANDLE FinishSignal;							  // Semaphore to signalize threads to abort.
		HANDLE FullQueue;								  // Semaphore which indicates how many (if any) sockets are enqueued on IR queue.
		CRITICAL_SECTION AcceptedSocketsAccess;			  // Critical section for accepted sockets access
		int socketsTaken = 0;
		int serverWorking = 1;

		//Init server thread struct
		SERVER_THREAD_DATA threadData;
		threadData.acceptedSocketsArray = acceptedSockets;
		threadData.clientInformationsMap = &clientInformationsMap;
		threadData.fileInfoMap = &fileInfoMap;
		threadData.FinishSignal = &FinishSignal;
		threadData.FullQueue = &FullQueue;
		threadData.incomingRequestsQueue = &incomingRequestsQueue;
		threadData.processingSocketsMap = &processingSocketsMap;
		threadData.socketsTaken = &socketsTaken;
		threadData.AcceptedSocketsAccess = &AcceptedSocketsAccess;
		threadData.serverWorking = &serverWorking;

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
		FullQueue = CreateSemaphore(0, 0, SERVER_THREADS, NULL);
		FinishSignal = CreateSemaphore(0, 0, SERVER_THREADS, NULL);

		//Init critical sections and threads if semaphores are ok
		if (FullQueue && FinishSignal)
		{
			InitializeCriticalSection(&AcceptedSocketsAccess);

			for (int i = 0; i < SERVER_THREADS; i++)
				processors[i] = CreateThread(NULL, 0, &ProcessIncomingFileRequest, (LPVOID)&threadData, 0, processorIDs + i);

			for (int i = 0; i < SERVER_THREADS; i++)
			{
				if (!processors[i]) {

					ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
					printf("\nReleased finish");

					for (int i = 0; i < SERVER_THREADS; i++)
						SAFE_DELETE_HANDLE(processors[i]);
					SAFE_DELETE_HANDLE(FullQueue);
					SAFE_DELETE_HANDLE(FinishSignal);

					DeleteCriticalSection(&AcceptedSocketsAccess);
					return 0;
				}
			}
		}

		InitClientInfoHandle();
		InitFileManagementHandle();



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

		while (serverWorking)
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
				int setSocketsCount = CheckSetSockets(&socketsTaken, acceptedSockets, &readfds, &incomingRequestsQueue, &processingSocketsMap);
				LeaveCriticalSection(&AcceptedSocketsAccess);
				if (setSocketsCount > 0)
				{
					printf("\n%d requests arrived.", setSocketsCount);
					ReleaseSemaphore(FullQueue, setSocketsCount, NULL);
				}


			}


		}
		ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
		for (int i = 0; i < SERVER_THREADS; i++)
			SAFE_DELETE_HANDLE(processors[i]);
		SAFE_DELETE_HANDLE(FullQueue);
		SAFE_DELETE_HANDLE(FinishSignal);

		DeleteCriticalSection(&AcceptedSocketsAccess);

		for (int i = 0; i < socketsTaken; i++)
		{
			ShutdownConnection(acceptedSockets + i);
		}

		// cleanup
		closesocket(listenSocket);
		WSACleanup();

		ClearClientInfoMap(&clientInformationsMap);
		ClearFileInfoMap(&fileInfoMap);

		DeleteClientInfoHandle();
		DeleteFileManagementHandle();

	}
	getchar();
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
	//Get server thread data
	SERVER_THREAD_DATA threadData = *((SERVER_THREAD_DATA*)param);
	Queue<SOCKET*>* incomingRequestsQueue = threadData.incomingRequestsQueue;
	SOCKET* acceptedSockets = threadData.acceptedSocketsArray;
	CRITICAL_SECTION* AcceptedSocketsAccess = threadData.AcceptedSocketsAccess;
	HashMap<SOCKET*>* processingSocketsMap = threadData.processingSocketsMap;
	HashMap<FILE_DATA>* fileInfoMap = threadData.fileInfoMap;
	HashMap<CLIENT_INFO>* clientInfoMap = threadData.clientInformationsMap;
	int* socketsTaken = threadData.socketsTaken;
	int* serverWorking = threadData.serverWorking;

	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { *(threadData.FinishSignal), *(threadData.FullQueue) };
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
		SOCKET* requestSocket;
		int getResult = incomingRequestsQueue->DequeueGet(&requestSocket);  //Get request from queue
		if (getResult == 0)
			continue;
		printf("\nDequeued request from queue");

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

			printf("\nClient %d has disconnected", requestSocket - acceptedSockets);
			CLIENT_INFO info;
			if (clientInfoMap->Get((const char*)requestSocket, &info))
			{
				printf("\nUnassigning client owned parts");
				if (UnassignFileParts(info.clientAddress, fileInfoMap, info.clientOwnedFiles, info.ownedFilesCount) != 0)
				{
					ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
					*serverWorking = 0;
					continue;
				}
			}
			result = RemoveClientInfo(requestSocket, clientInfoMap);
			EnterCriticalSection(AcceptedSocketsAccess);
			result += ShutdownConnection(requestSocket);
			processingSocketsMap->Delete((const char*)(requestSocket));
			result += RemoveSocketFromArray(acceptedSockets, requestSocket, socketsTaken);
			LeaveCriticalSection(AcceptedSocketsAccess);
			if (result != 0) //Shutdown everything in case something went terribly wrong
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				*serverWorking = 0;
			}
			printf("\nRemoved client!");
			continue;

		}

		//Remove socket from processing map
		processingSocketsMap->Delete((const char*)(requestSocket));

		int isFileLoaded = fileInfoMap->DoesKeyExist(fileRequest.fileName);
		//LeaveCriticalSection(&FileMapAccess);

		if (isFileLoaded)//File is loaded and can be given back to client
		{
			printf("File %s is already loaded in memory.", fileRequest.fileName);
			fileInfoMap->Get(fileRequest.fileName, &fileData);
			int result = PackExistingFileResponse(&fileResponse, fileData, fileRequest, serverOwnedParts, fileInfoMap);
			if (result != 0) //Shutdown everything in case something went terribly wrong
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				*serverWorking = 0;
				continue;
			}
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
				printf("\nFile %s doesn't exist on server", fileRequest.fileName);
				fileResponse.fileExists = 0;
				fileResponse.clientPartsNumber = 0;
				fileResponse.filePartToStore = 0;
				fileResponse.serverPartsNumber = 0;
				fileResponse.fileSize = 0;
				isAssignedWithPart = -1;
			}
			else //File is on server and loaded into buffer
			{
				printf("\nFile %s is now loaded into buffer", fileRequest.fileName);
				int resultOp = DivideFileIntoParts(fileBuffer, fileSize, FILE_PARTS, &fileParts);
				if (resultOp != 0) //Shutdown everything in case something went terribly wrong
				{
					ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
					*serverWorking = 0;
					continue;
				}
				fileResponse.fileExists = 1;
				fileResponse.clientPartsNumber = 0;
				fileResponse.filePartToStore = 0;
				fileResponse.serverPartsNumber = FILE_PARTS;
				fileResponse.fileSize = fileSize;

				//Add server owned part indexes
				for (int i = 0; i < FILE_PARTS; i++)serverOwnedParts[i] = i;

				//Init new File data structure for newly loaded file
				fileData.filePointer = fileBuffer;
				fileData.filePartDataArray = fileParts;
				fileData.partArraySize = 2 * FILE_PARTS;
				fileData.nextPartToAssign = 0;
				memcpy(fileData.fileName, fileRequest.fileName, MAX_FILE_NAME);
				fileData.partsOnClients = 0;
				fileData.fileSize = fileSize;

				//Add new file data structure to map, no need for CS because no one owns this structure yet
				fileInfoMap->Insert(fileRequest.fileName, fileData);
			}

		}

		if (SendFileResponse(*requestSocket, fileResponse) != 0)
		{
			printf("\nClient %d has disconnected", requestSocket - acceptedSockets);
			CLIENT_INFO info;
			if (clientInfoMap->Get((const char*)requestSocket, &info))
			{
				printf("\nUnassigning client owned parts");
				if (UnassignFileParts(info.clientAddress, fileInfoMap, info.clientOwnedFiles, info.ownedFilesCount) != 0)
				{
					ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
					*serverWorking = 0;
					continue;
				}
			}
			result = RemoveClientInfo(requestSocket, clientInfoMap);
			EnterCriticalSection(AcceptedSocketsAccess);
			result += ShutdownConnection(requestSocket);
			result += RemoveSocketFromArray(acceptedSockets, requestSocket, socketsTaken);
			LeaveCriticalSection(AcceptedSocketsAccess);
			if (result != 0) //Shutdown everything in case something went terribly wrong
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				*serverWorking = 0;
			}
			printf("\nRemoved client!");
			continue;
		}

		int sockERR = 0;
		for (int i = 0; i < (int)fileResponse.serverPartsNumber; i++)
		{
			int partIndex = serverOwnedParts[i];
			FILE_PART partToSend = fileParts[partIndex];
			if (SendFilePart(*requestSocket, partToSend.partStartPointer, partToSend.partSize, partIndex) != 0)
			{
				printf("\nFailed to send part number %d. Shutting down connection to client. Error %ld", i, WSAGetLastError());
				CLIENT_INFO info;
				if (clientInfoMap->Get((const char*)requestSocket, &info))
				{
					printf("\nUnassigning client owned parts");
					if (UnassignFileParts(info.clientAddress, fileInfoMap, info.clientOwnedFiles, info.ownedFilesCount) != 0)
					{
						ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
						*serverWorking = 0;
						continue;
					}
				}
				result = RemoveClientInfo(requestSocket, clientInfoMap);
				EnterCriticalSection(AcceptedSocketsAccess);
				result += ShutdownConnection(requestSocket);
				result += RemoveSocketFromArray(acceptedSockets, requestSocket, socketsTaken);
				LeaveCriticalSection(AcceptedSocketsAccess);
				if (result != 0) //Shutdown everything in case something went terribly wrong
				{
					ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
					*serverWorking = 0;
				}
				printf("\nRemoved client!");
				sockERR = 1;
				break;
			}
		}
		if (sockERR == 1) {
			continue; //Skip processing as socket was compromised
		}
		if (isAssignedWithPart == 0)
		{
			if (AssignFilePartToClient(fileRequest.requesterListenAddress, fileRequest.fileName, fileInfoMap) < 0)
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				*serverWorking = 0;
				continue;
			}
		}
		if (isAssignedWithPart != -1)
		{
			if (AddClientInfo(requestSocket, fileData, fileRequest.requesterListenAddress, clientInfoMap))
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				*serverWorking = 0;
			}
		}
	}
	printf("\nThread finished.");

	
	return 0;
}



