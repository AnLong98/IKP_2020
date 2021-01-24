#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#include "../DataStructures/HashMap.h"
#include "../DataStructures/Queue.h"
#include "../DataStructures/LinkedList.h"
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
#define SOCKET_BUFFER_SIZE 20
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

bool InitializeWindowsSockets();
DWORD WINAPI ProcessIncomingFileRequest(LPVOID param);
DWORD WINAPI ProcessAdminInput(LPVOID param);


int  main(void)
{
	{
		//Server data structures
		HashMap<SOCKET> processingSocketsMap;
		HashMap<FILE_DATA> fileInfoMap;
		HashMap<int> loadingFilesMap;					//Hash map that stores names of file that are being loaded by server threads, used to avoid double loads
		Queue<SOCKET> incomingRequestsQueue(MAX_QUEUE);
		HashMap<CLIENT_INFO> clientInformationsMap;
		LinkedList<SOCKET> acceptedSockets;
		HANDLE FinishSignal;							  // Semaphore to signalize threads to abort.
		HANDLE FullQueue;								  // Semaphore which indicates how many (if any) sockets are enqueued on IR queue.
		CRITICAL_SECTION LoadingFilesAccess;			  // Critical section for access to loading files structs
		int serverWorking = 1;
		int socketsTaken = 0;

		// Socket used for listening for new clients 
		SOCKET listenSocket = INVALID_SOCKET;
		// Socket used for communication with client

		// variable used to store function return value
		int iResult;
		// Buffer used for storing incoming data
		fd_set readfds;
		unsigned long mode = 1; //non-blocking mode

		HANDLE processors[SERVER_THREADS] = { NULL };
		HANDLE serverController = NULL;
		DWORD processorIDs[SERVER_THREADS];
		DWORD serverControllerID;


		//Create Semaphores
		FullQueue = CreateSemaphore(0, 0, SERVER_THREADS, NULL);
		FinishSignal = CreateSemaphore(0, 0, SERVER_THREADS, NULL);

		//Init critical sections and threads if semaphores are ok
		if (FullQueue && FinishSignal)
		{
			InitializeCriticalSection(&LoadingFilesAccess);

			//Init server thread struct
			SERVER_THREAD_DATA threadData;
			threadData.acceptedSocketsArray = &acceptedSockets;
			threadData.clientInformationsMap = &clientInformationsMap;
			threadData.fileInfoMap = &fileInfoMap;
			threadData.FinishSignal = &FinishSignal;
			threadData.FullQueue = &FullQueue;
			threadData.incomingRequestsQueue = &incomingRequestsQueue;
			threadData.processingSocketsMap = &processingSocketsMap;
			threadData.LoadingFilesAccess = &LoadingFilesAccess;
			threadData.serverWorking = &serverWorking;
			threadData.loadingFilesMap = &loadingFilesMap;

			//Init server controller struct
			SERVER_CONTROLLER_THREAD_DATA controllerData;
			controllerData.FinishSignal = &FinishSignal;
			controllerData.serverWorking = &serverWorking;


			for (int i = 0; i < SERVER_THREADS; i++)
				processors[i] = CreateThread(NULL, 0, &ProcessIncomingFileRequest, (LPVOID)&threadData, 0, processorIDs + i);
			
			serverController = CreateThread(NULL, 0, &ProcessAdminInput, (LPVOID)&controllerData, 0, &serverControllerID);

			for (int i = 0; i < SERVER_THREADS; i++)
			{
				if (!processors[i] || !serverController) {

					ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
					printf("\nReleased finish");

					for (int i = 0; i < SERVER_THREADS; i++)
						SAFE_DELETE_HANDLE(processors[i]);
					SAFE_DELETE_HANDLE(serverController);
					SAFE_DELETE_HANDLE(FullQueue);
					SAFE_DELETE_HANDLE(FinishSignal);

					DeleteCriticalSection(&LoadingFilesAccess);
					return 0;
				}
			}
		}

		InitClientInfoHandle();
		InitFileManagementHandle();
		InitConnectionsHandle();



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

		printf("Server initialized, waiting for clients.\n Press Q to close\n");
		while (serverWorking)
		{
			FD_ZERO(&readfds);
			FD_SET(listenSocket, &readfds);

			int socketsCount = 0;
			SOCKET* sockets = GetAllSockets(&acceptedSockets, &socketsCount);
			socketsTaken = socketsCount;
			for (int i = 0; i < socketsCount; i++)
			{
				//Convert socket to string to use as key
				char socketBuffer[SOCKET_BUFFER_SIZE];
				sprintf_s(socketBuffer, "%d", sockets[i]);

				if(!processingSocketsMap.DoesKeyExist(socketBuffer))
					FD_SET(sockets[i], &readfds);
			}

			if (sockets != NULL)
				free(sockets);

			timeval timeVal;
			timeVal.tv_sec = 3;
			timeVal.tv_usec = 0;

			int result = select(0, &readfds, NULL, NULL, &timeVal); //
			

			if (result == 0) {
				continue;
			}
			else if (result == SOCKET_ERROR) {
				printf("\nError detected on select, sleeping for 100ms to let worker threads handle");
				Sleep(100);
				continue;	
			}
			else {
				if (FD_ISSET(listenSocket, &readfds) && socketsTaken < MAX_CLIENTS)
				{
					if (AcceptIncomingConnection(&acceptedSockets, listenSocket) == 0)
					{
						printf("\nAccepted Connection");
						socketsTaken++;
					}
					else
					{
						printf("\nAccepting new connection failed");
						serverWorking = 0;
						break;
					}
					
				}

				int setSocketsCount = CheckSetSockets(&acceptedSockets, &readfds, &incomingRequestsQueue, &processingSocketsMap);
				if (setSocketsCount > 0)
				{
					printf("\n%d requests arrived.", setSocketsCount);
					ReleaseSemaphore(FullQueue, setSocketsCount, NULL);
				}


			}


		}
		printf("\nServer is not listening any more");
		for (int i = 0; i < SERVER_THREADS; i++)
			WaitForSingleObject(processors[i], 2000);

		WaitForSingleObject(serverController, 2000);


		for (int i = 0; i < SERVER_THREADS; i++)
			SAFE_DELETE_HANDLE(processors[i]);
		SAFE_DELETE_HANDLE(serverController);
		SAFE_DELETE_HANDLE(FullQueue);
		SAFE_DELETE_HANDLE(FinishSignal);

		DeleteCriticalSection(&LoadingFilesAccess);

		int socketsCount = 0;
		SOCKET* sockets = GetAllSockets(&acceptedSockets, &socketsCount);
		for (int i = 0; i < socketsCount; i++)
		{
			ShutdownConnection(sockets[i]);
		}

		if (sockets!= NULL)
			free(sockets);
		// cleanup
		closesocket(listenSocket);
		WSACleanup();

		ClearClientInfoMap(&clientInformationsMap);
		ClearFileInfoMap(&fileInfoMap);

		DeleteClientInfoHandle();
		DeleteFileManagementHandle();
		DeleteConnectionsHandle();

	}
	printf("\nPress any key to close");
	_getch();
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
	Queue<SOCKET>* incomingRequestsQueue = threadData.incomingRequestsQueue;
	LinkedList<SOCKET>* acceptedSockets = threadData.acceptedSocketsArray;
	HashMap<SOCKET>* processingSocketsMap = threadData.processingSocketsMap;
	HashMap<FILE_DATA>* fileInfoMap = threadData.fileInfoMap;
	HashMap<CLIENT_INFO>* clientInfoMap = threadData.clientInformationsMap;
	HashMap<int>* loadingFilesMap = threadData.loadingFilesMap;
	int* serverWorking = threadData.serverWorking;

	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { *(threadData.FinishSignal), *(threadData.FullQueue) };
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1) {
		SOCKET requestSocket;
		int getResult = incomingRequestsQueue->DequeueGet(&requestSocket);  //Get request from queue
		if (getResult == 0)
			continue;
		printf("\nDequeued request from queue");

		//Convert socket to string to use as key
		char socketBuffer[SOCKET_BUFFER_SIZE];
		sprintf_s(socketBuffer, "%d", requestSocket);

		FILE_DATA fileData;
		FILE_PART* fileParts = NULL;
		FILE_PART filePartsSend[FILE_PARTS];
		FILE_REQUEST fileRequest;
		FILE_RESPONSE fileResponse;
		int serverOwnedParts[FILE_PARTS];
		int isAssignedWithPart = 0;

		//Receive request from socket
		int result = RecvFileRequest(requestSocket, &fileRequest);
		if (result == -1)
		{

			printf("\nClient has disconnected while trying to receive file reuest");
			CLIENT_INFO info;
			if (clientInfoMap->Get((const char*)socketBuffer, &info))
			{
				printf("\nUnassigning client owned parts");
				UnassignFileParts(info.clientAddress, fileInfoMap, info, info.ownedFilesCount);
			}
			RemoveClientInfo(requestSocket, clientInfoMap);
			result = ShutdownConnection(requestSocket);
			result += RemoveSocketFromArray(acceptedSockets, requestSocket);
			if (result != 0) //Shutdown everything in case something went terribly wrong
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				printf("\nRemove client failed failed");
				*serverWorking = 0;
			}
			processingSocketsMap->Delete((const char*)(socketBuffer));
			printf("\nRemoved client!");
			continue;

		}

		EnterCriticalSection(threadData.LoadingFilesAccess);
		int isBeingLoaded = loadingFilesMap->DoesKeyExist(fileRequest.fileName);
		int isFileLoaded = fileInfoMap->DoesKeyExist(fileRequest.fileName);
		if (!isBeingLoaded && !isFileLoaded) //We will load it if no one is doing it at the moment
		{
			loadingFilesMap->Insert(fileRequest.fileName, 1);
		}
		LeaveCriticalSection(threadData.LoadingFilesAccess);
		while (isBeingLoaded) //We need to wait for other threads to load this file
		{
			if (!*serverWorking)return 0;
			Sleep(1000);
			isBeingLoaded = loadingFilesMap->DoesKeyExist(fileRequest.fileName);
		}
		
		isFileLoaded = fileInfoMap->DoesKeyExist(fileRequest.fileName);
		if (isFileLoaded)//File is loaded and can be given back to client
		{
			printf("\nFile %s is already loaded in memory.", fileRequest.fileName);
			int result = PackExistingFileResponse(&fileResponse, fileRequest.fileName, fileRequest, serverOwnedParts, fileInfoMap, filePartsSend);
			if (result != 0) //Shutdown everything in case something went terribly wrong
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				printf("\nPack existing response failed");
				*serverWorking = 0;
				continue;
			}
			isAssignedWithPart = 1;

		}
		else // We need to load the file first
		{
			char* fileBuffer = NULL;
			size_t fileSize;
			int result = ReadFileIntoMemory(fileRequest.fileName, &fileBuffer, &fileSize);

			if (result != 0)//File probably doesn't exist on server
			{
				loadingFilesMap->Delete(fileRequest.fileName); // Tell other threads that file is not being loaded anymore
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
					printf("\nDivide file parts failed");
					*serverWorking = 0;
					continue;
				}
				fileResponse.fileExists = 1;
				fileResponse.clientPartsNumber = 0;
				fileResponse.filePartToStore = 0;
				fileResponse.serverPartsNumber = FILE_PARTS;
				fileResponse.fileSize = fileSize;

				//Add server owned part indexes and set copies of file parts
				for (int i = 0; i < FILE_PARTS; i++)
				{
					filePartsSend[i] = fileParts[i];
					serverOwnedParts[i] = i;
				}

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
				loadingFilesMap->Delete(fileRequest.fileName); // Tell other threads that file is loaded
			}

		}

		if (SendFileResponse(requestSocket, fileResponse) != 0)
		{
			printf("\nClient has disconnected while sending file response");
			CLIENT_INFO info;
			if (clientInfoMap->Get((const char*)socketBuffer, &info))
			{
				printf("\nUnassigning client owned parts as response failed");
				UnassignFileParts(info.clientAddress, fileInfoMap, info, info.ownedFilesCount);

			}
			RemoveClientInfo(requestSocket, clientInfoMap);
			result = ShutdownConnection(requestSocket);
			result += RemoveSocketFromArray(acceptedSockets, requestSocket);
			if (result != 0) //Shutdown everything in case something went terribly wrong
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				printf("\nRemove client failed");
				*serverWorking = 0;
			}
			printf("\nRemoved client!");
			continue;
		}

		int sockERR = 0;
		for (int i = 0; i < (int)fileResponse.serverPartsNumber; i++)
		{
			int partIndex = serverOwnedParts[i];
			FILE_PART partToSend = filePartsSend[partIndex];
			if (SendFilePart(requestSocket, partToSend.partStartPointer, partToSend.partSize, partIndex) != 0)
			{
				printf("\nFailed to send part number %d. Shutting down connection to client. Error %ld", i, WSAGetLastError());
				CLIENT_INFO info;
				if (clientInfoMap->Get((const char*)socketBuffer, &info))
				{
					printf("\nUnassigning client owned parts as send part failed");
					UnassignFileParts(info.clientAddress, fileInfoMap, info, info.ownedFilesCount);

				}
				RemoveClientInfo(requestSocket, clientInfoMap);
				result = ShutdownConnection(requestSocket);
				result += RemoveSocketFromArray(acceptedSockets, requestSocket);
				if (result != 0) //Shutdown everything in case something went terribly wrong
				{
					ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
					printf("\nClient remove failed");
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
				printf("\nAssign file part failed");
				*serverWorking = 0;
				continue;
			}
		}

		if (isAssignedWithPart != -1)
		{
			if(AddClientInfo(requestSocket, fileRequest.fileName, fileRequest.requesterListenAddress, clientInfoMap) != 0)
			{
				ReleaseSemaphore(*(threadData.FinishSignal), SERVER_THREADS, NULL);
				printf("\nAdd client info failed");
				*serverWorking = 0;
				continue;
			}
		}
		//Remove socket from processing map
		processingSocketsMap->Delete((const char*)(socketBuffer));
	
	}
	printf("\nThread finished.");

	
	return 0;
}


DWORD WINAPI ProcessAdminInput(LPVOID param)
{
	SERVER_CONTROLLER_THREAD_DATA data = *((SERVER_CONTROLLER_THREAD_DATA*)param);
	int* serverWorking = data.serverWorking;
	while (serverWorking)
	{
		if (_kbhit())
		{
			char ch = _getch();
			if (ch == 'Q' or ch == 'q')
			{
				*serverWorking = 0;
				ReleaseSemaphore(*(data.FinishSignal), SERVER_THREADS, NULL);
				printf("\n\n-----Server shutting down as per your command------\n\n");
				continue;
			}
		}
		Sleep(100); //Sleep to avoid 
	}

	return 0;
}



