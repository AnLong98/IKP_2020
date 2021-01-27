// Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#pragma comment(lib, "Ws2_32.lib")
#include <stdio.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <conio.h>
#include <time.h>
#include "../DataStructures/HashMap.h"
#include "../DataStructures/Queue.h"
#include "../DataStructures/LinkedList.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"
#include "../PeerToPeerFileTransferFunctions/P2PLimitations.h"
#include "../FileIO_Functions/FileIO.h"
#include "../ClientData/Client_Structs.h"
#include "../ClientData/FileManager.h"
#include "../FTPServerFunctions/ConnectionFuncs.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS 1
#define SOCKET_BUFFER_SIZE 20
#define DEFAULT_PORT 27016
#define SERVER_ADDRESS "127.0.0.1"
#define CLIENT_THREADS 5
#define OUT_QUEUE_THREADS 3
#define INC_QUEUE_THREADS 2
#define SERVER_THREAD 1
#define ALL_THREADS 6
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

bool InitializeWindowsSockets();
DWORD WINAPI ProcessConnectionToServer(LPVOID param);
DWORD WINAPI ProcessOutgoingFilePartRequest(LPVOID param);
DWORD WINAPI ProccessIncomingFilePartRequest(LPVOID param);


int main(void)
{

	{
		//Server and OutgoingFilePartRequest data
		Queue<CLIENT_FILE_PART> outgoingRequestQueue; //Queue for Outgoing Requests to other clients.
		SOCKET listenSocket;
		CLIENT_DOWNLOADING_FILE wholeFile;			  //Place to assemble file parts so we can write it to memory.
		HANDLE EmptyOutgoingQueue;					  
		HANDLE FullOutgoingQueue;

		//IncomingFilePartRequest data
		Queue<SOCKET> incomingRequestQueue;				//Queue for Incoming Requests from other clients.
		HashMap<SOCKET> processingSocketsMap;
		HANDLE FullIncomingQueue;
		LinkedList<SOCKET> acceptedSockets;
		int socketsTaken = 0;

		//Common data
		HANDLE FinishSignal;						  //Semaphore for signalizing abort to threads.
		LinkedList<CLIENT_FILE_PART_INFO> fileParts;  //LinkedList where we store parts that we need to provide to other clients.
		int shutDownClient = 1;


		if (InitializeWindowsSockets() == false)
		{
			return 1;
		}

		listenSocket = INVALID_SOCKET;
		int iResult;

		fd_set readfds;
		unsigned long mode = 1;
		addrinfo *resultingAddress = NULL;
		addrinfo hints;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		iResult = getaddrinfo(NULL, "0", &hints, &resultingAddress);
		if (iResult != 0)
		{
			printf("getaddrinfo failed with error: %d\n", iResult);
			WSACleanup();
			return 1;
		}

		listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (listenSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld\n", WSAGetLastError());
			freeaddrinfo(resultingAddress);
			WSACleanup();
			return 1;
		}

		iResult = bind(listenSocket, resultingAddress->ai_addr, (int)resultingAddress->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("bind failed with error: %d\n", WSAGetLastError());
			freeaddrinfo(resultingAddress);
			closesocket(listenSocket);
			WSACleanup();
			return 1;
		}

		freeaddrinfo(resultingAddress);

		HANDLE servProc = NULL;
		DWORD servProcID;

		HANDLE processors[CLIENT_THREADS] = { NULL };
		DWORD processorIDs[CLIENT_THREADS];

		EmptyOutgoingQueue = CreateSemaphore(0, OUT_QUEUE_THREADS, OUT_QUEUE_THREADS, NULL);
		FullOutgoingQueue = CreateSemaphore(0, 0, OUT_QUEUE_THREADS, NULL);

		FullIncomingQueue = CreateSemaphore(0, 0, INC_QUEUE_THREADS, NULL);

		FinishSignal = CreateSemaphore(0, 0, ALL_THREADS, NULL);

		OUT_AND_SERVER_THREAD_DATA outServerThreadData;

		if (EmptyOutgoingQueue && FullOutgoingQueue && FullIncomingQueue && FinishSignal)
		{
			//init out and server thread struct
			outServerThreadData.outgoingRequestQueue = &outgoingRequestQueue;
			outServerThreadData.listenSocket = &listenSocket;
			outServerThreadData.fileParts = &fileParts;
			outServerThreadData.wholeFile = &wholeFile;
			outServerThreadData.FinishSignal = &FinishSignal;
			outServerThreadData.EmptyOutgoingQueue = &EmptyOutgoingQueue;
			outServerThreadData.FullOutgoingQueue = &FullOutgoingQueue;
			outServerThreadData.shutDownClient = &shutDownClient;

			//init inc thread data
			INC_THREAD_DATA incThreadData;
			incThreadData.incomingRequestQueue = &incomingRequestQueue;
			incThreadData.fileParts = &fileParts;
			incThreadData.processingSocketsMap = &processingSocketsMap;
			incThreadData.FinishSignal = &FinishSignal;
			incThreadData.FullIncomingQueue = &FullIncomingQueue;
			incThreadData.shutDownClient = &shutDownClient;
			incThreadData.acceptedSockets = &acceptedSockets;

			for (int i = 0; i < OUT_QUEUE_THREADS; i++)
			{
				processors[i] = CreateThread(NULL, 0, &ProcessOutgoingFilePartRequest, (LPVOID)&outServerThreadData, 0, processorIDs + i);
			}

			for (int i = OUT_QUEUE_THREADS; i < CLIENT_THREADS; i++)
			{
				processors[i] = CreateThread(NULL, 0, &ProccessIncomingFilePartRequest, (LPVOID)&incThreadData, 0, processorIDs + i);
			}

			for (int i = 0; i < CLIENT_THREADS; i++)
			{
				if (!processors[i])
				{
					ReleaseSemaphore(FinishSignal, ALL_THREADS, NULL);

					for (int i = 0; i < CLIENT_THREADS; i++)
						SAFE_DELETE_HANDLE(processors[i]);

					SAFE_DELETE_HANDLE(EmptyOutgoingQueue);
					SAFE_DELETE_HANDLE(FullOutgoingQueue);
					SAFE_DELETE_HANDLE(FullIncomingQueue);
					SAFE_DELETE_HANDLE(FinishSignal);

					printf("\nReleased FinishSignal in 1st MAIN");

					return 0;
				}
			}
		}

		//Init handles
		InitConnectionsHandle();
		InitWholeFileManagementHandle();
		InitFileAcessManagementHandle();

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

		printf("Client is up...");

		struct sockaddr_in socketAddress;
		int socketAddress_len = sizeof(socketAddress);

		if (getsockname(listenSocket, (sockaddr *)&socketAddress, &socketAddress_len) == -1)
		{
			printf("getsockname() failed.\n"); return -1;
		}
		//Creating thread for communication with server
		servProc = CreateThread(NULL, 0, &ProcessConnectionToServer, (LPVOID)&outServerThreadData, 0, &servProcID);

		if (!servProc)
		{
			ReleaseSemaphore(FinishSignal, ALL_THREADS, NULL);

			for (int i = 0; i < CLIENT_THREADS; i++)
				SAFE_DELETE_HANDLE(processors[i]);

			SAFE_DELETE_HANDLE(servProc);

			SAFE_DELETE_HANDLE(EmptyOutgoingQueue);
			SAFE_DELETE_HANDLE(FullOutgoingQueue);
			SAFE_DELETE_HANDLE(FullIncomingQueue);
			SAFE_DELETE_HANDLE(FinishSignal);

			printf("\nReleased FinishSignal");

			return 0;
		}


		while (shutDownClient)
		{
			FD_ZERO(&readfds);
			FD_SET(listenSocket, &readfds);

			int socketsCount = 0;
			SOCKET *sockets = GetAllSockets(&acceptedSockets, &socketsCount);
			socketsTaken = socketsCount;

			for (int i = 0; i < socketsTaken; i++)
			{
				//Convert socket to string to use as key
				char socketBuffer[SOCKET_BUFFER_SIZE];
				sprintf_s(socketBuffer, "%d", sockets[i]);

				if (!processingSocketsMap.DoesKeyExist(socketBuffer))
					FD_SET(sockets[i], &readfds);

			}

			if (sockets != NULL)
				free(sockets);

			timeval timeVal;
			timeVal.tv_sec = 3;
			timeVal.tv_usec = 0;

			int result = select(0, &readfds, NULL, NULL, &timeVal);

			if (result == 0)
			{
				continue;
			}
			else if (result == SOCKET_ERROR)
			{
				printf("\nError detected on select.");
				Sleep(100);
				continue;
			}
			else
			{
				if (FD_ISSET(listenSocket, &readfds))
				{
					if (AcceptIncomingConnection(&acceptedSockets, listenSocket) == 0)
					{
						printf("\nAccepted connection.");
						socketsTaken++;
					}
					else
					{
						printf("\nCouldn't accept new connection");
						shutDownClient = 0;
						break;
					}
				}

				int setSocketsCount = CheckSetSockets(&acceptedSockets, &readfds, &incomingRequestQueue, &processingSocketsMap);
				if (setSocketsCount > 0)
				{
					printf("\n%d requests arrived.", setSocketsCount);
					int socketsSignaled = 0;
					for (int i = 0; i < setSocketsCount; i++)
					{
						while (!ReleaseSemaphore(FullIncomingQueue, 1, NULL))
						{
							Sleep(200);
						}
					}
				}

			}
		}

		printf("\nClient is not listening any more");

		for (int i = 0; i < CLIENT_THREADS; i++)
			WaitForSingleObject(processors[i], 2000);

		WaitForSingleObject(servProc, 2000);

		for (int i = 0; i < CLIENT_THREADS; i++)
			SAFE_DELETE_HANDLE(processors[i]);

		SAFE_DELETE_HANDLE(servProc);

		SAFE_DELETE_HANDLE(EmptyOutgoingQueue);
		SAFE_DELETE_HANDLE(FullOutgoingQueue);
		SAFE_DELETE_HANDLE(FullIncomingQueue);
		SAFE_DELETE_HANDLE(FinishSignal);


		int socketsCount = 0;
		SOCKET* sockets = GetAllSockets(&acceptedSockets, &socketsCount);

		for (int i = 0; i < socketsCount; i++)
		{
			ShutdownConnection(sockets[i]);
		}

		if (sockets != NULL)
			free(sockets);

		// cleanup
		closesocket(listenSocket);
		    
		ClearFilePartsLinkedList(&fileParts); 
		//ResetWholeFile(&wholeFile);
		
		DeleteWholeFileManagementHandle();
		DeleteFileAccessManagementHandle();
		DeleteConnectionsHandle();

		WSACleanup();
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

DWORD WINAPI ProcessConnectionToServer(LPVOID param)
{
	//take outServerThreadData
	OUT_AND_SERVER_THREAD_DATA outServerThreadData = *((OUT_AND_SERVER_THREAD_DATA*)param);
	Queue<CLIENT_FILE_PART>* outgoingRequestQueue = outServerThreadData.outgoingRequestQueue; 
	SOCKET* listenSocket = outServerThreadData.listenSocket;
	CLIENT_DOWNLOADING_FILE* wholeFile = outServerThreadData.wholeFile;
	LinkedList<CLIENT_FILE_PART_INFO>* fileParts = outServerThreadData.fileParts;
	int* shutDownClient = outServerThreadData.shutDownClient;

	SOCKET connectSocket = INVALID_SOCKET;

	connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	InetPton(AF_INET, TEXT(SERVER_ADDRESS), &serverAddress.sin_addr.s_addr);
	serverAddress.sin_port = htons(DEFAULT_PORT);

	//Conect to server
	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		*shutDownClient = 0;
		ReleaseSemaphore(*(outServerThreadData.FinishSignal), ALL_THREADS, NULL);
		return 0;
	}

	struct sockaddr_in socketAddress;
	int socketAddress_len = sizeof(socketAddress);

	if (getsockname(*listenSocket, (sockaddr *)&socketAddress, &socketAddress_len) == -1)
	{
		printf("getsockname() failed.\n");
		return -1;
	}

	FILE_REQUEST file;
	char filename[MAX_FILE_NAME] = { 0 };

	while (*shutDownClient)
	{
		printf("\n\n---Example: stefan.txt ---\nEnter a file name: ");
		gets_s(file.fileName, MAX_FILE_NAME);

		InetPton(AF_INET, TEXT("127.0.0.1"), &socketAddress.sin_addr.s_addr);

		file.requesterListenAddress = socketAddress;
		printf("\n\n--------PORT-----------");
		printf("%d\n\n", socketAddress.sin_port);


		//Sending request for file to server
		int result = SendFileRequest(connectSocket, file);
		if (result == -1) //Shuting down client if server is not working
		{
			printf("\nServer has disconnected while trying to send file request.");
			ShutdownConnection(connectSocket);
			//ReleaseSemaphore(*(outServerThreadData.FinishSignal), ALL_THREADS, NULL);
			printf("\nServer is down. Shuting down client.");
			*shutDownClient = 0;
			break;
		}

		printf("\nFiles sent to server. Waiting for response...");

		FILE_RESPONSE response;

		//Recive response from server
		if (RecvFileResponse(connectSocket, &response) == -1) //Shuting down client if server is not working
		{
			printf("\nServer has disconnected while trying to recieve file response.");
			ShutdownConnection(connectSocket);
			//ReleaseSemaphore(*(outServerThreadData.FinishSignal), ALL_THREADS, NULL);
			printf("\nServer is down. Shuting down client.");
			*shutDownClient = 0;
			break;
		}

		if (!response.fileExists)
		{
			printf("\nFile does not exist on server. Try again with new file name.");
			continue;
		}

		printf("\nResponse recieved from server. Waiting for parts from server...");

		InitWholeFile(wholeFile, file.fileName, response);

		CLIENT_FILE_PART clientFilePart;

		//Activating thread if parts need to be taken from other clients
		for (int i = 0; i < (int)response.clientPartsNumber; i++)
		{
			const int semaphoreNum = 2;
			HANDLE semaphores[semaphoreNum] = { *(outServerThreadData.FinishSignal), *(outServerThreadData.EmptyOutgoingQueue) };
			DWORD waitResult = WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE);

			if (waitResult == WAIT_OBJECT_0 + 1)
			{
				printf("\nEmptyOutQueue taken");

				memcpy(clientFilePart.fileName, file.fileName, MAX_FILE_NAME);
				clientFilePart.filePartInfo.clientOwnerAddress = response.clientParts[i].clientOwnerAddress;
				clientFilePart.filePartInfo.partNumber = response.clientParts[i].partNumber;

				outgoingRequestQueue->Enqueue(clientFilePart);

				ReleaseSemaphore(*(outServerThreadData.FullOutgoingQueue), 1, NULL);
				printf("\nFullOutQueue released");
			}
		}

		CLIENT_FILE_PART_INFO part;

		char* data = NULL;
		unsigned int length;
		int partNumber;

		// Recive file parts from server
		for (int i = 0; i < (int)response.serverPartsNumber; i++)
		{
			if (RecvFilePart(connectSocket, &data, &length, &partNumber) == -1) //Shuting down Client if server is down
			{
				printf("\nServer has disconnected while trying to recieve file part.");
				ShutdownConnection(connectSocket);
				//ReleaseSemaphore(*(outServerThreadData.FinishSignal), ALL_THREADS, NULL);
				printf("\nServer is down. Shuting down client.");
				*shutDownClient = 0;
				break;
			}

			HandleRecievedFilePart(wholeFile, data, length, partNumber, fileParts);

			free(data);
			data = NULL;
		}

		//Wait for all parts to be downloaded
		while (wholeFile->partsDownloaded != FILE_PARTS)
		{
			Sleep(500);
		}

		//Save file to disk
		char ip[] = "127.0.0.1-";
		char port[10];
		sprintf(port, "%d", socketAddress.sin_port);

		char* dirName = (char*)malloc(strlen(ip) + strlen(port) + 2);
		strcpy_s(dirName,strlen(ip) + 1, ip);
		strcat(dirName, port);

		if (WriteWholeFileIntoMemory(dirName, *wholeFile) != 0)
		{
			printf("\nCouldn't write buffer into memory."); //koji jos hendle baciti ovde, mislim da je okej da se izvrsi ovo dole i da mu dozvolimo da opet upise fajl koji zeli 
		}

		free(dirName);
		dirName = NULL;
		ResetWholeFile(wholeFile); //unutra ne treba kriticna sekcija. proveriti?????? 
	}

	ReleaseSemaphore(*(outServerThreadData.FinishSignal), ALL_THREADS, NULL);
	printf("\nServer communication thread has finished with work.");
	return 0;
}

DWORD WINAPI ProcessOutgoingFilePartRequest(LPVOID param)
{	
	//take outServerThreadData
	OUT_AND_SERVER_THREAD_DATA outServerThreadData = *((OUT_AND_SERVER_THREAD_DATA*)param);
	Queue<CLIENT_FILE_PART>* outgoingRequestQueue = outServerThreadData.outgoingRequestQueue;
	SOCKET* listenSocket = outServerThreadData.listenSocket;
	CLIENT_DOWNLOADING_FILE* wholeFile = outServerThreadData.wholeFile;
	LinkedList<CLIENT_FILE_PART_INFO>* fileParts = outServerThreadData.fileParts;
	int *shutDownClient = outServerThreadData.shutDownClient;


	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { *(outServerThreadData.FinishSignal), *(outServerThreadData.FullOutgoingQueue) };
	
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
	{
		printf("\nEntered in FullOutgoingRequestQueue");

		CLIENT_FILE_PART filePart;
		outgoingRequestQueue->DequeueGet(&filePart);

		FILE_PART_REQUEST partRequest;
		memcpy(partRequest.fileName, filePart.fileName, MAX_FILE_NAME);
		partRequest.partNumber = filePart.filePartInfo.partNumber;

		SOCKET connectSocket = INVALID_SOCKET;

		connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (connectSocket == INVALID_SOCKET)
		{
			printf("socket failed with error: %ld", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		sockaddr_in clientAddress = filePart.filePartInfo.clientOwnerAddress;

		//Unable to connect to client, release semaphore
		if (connect(connectSocket, (SOCKADDR*)&clientAddress, sizeof(clientAddress)) == SOCKET_ERROR)
		{
			printf("Unable to connect to client.\n");
			closesocket(connectSocket);
			ReleaseSemaphore(*(outServerThreadData.EmptyOutgoingQueue), 1, NULL);
			//return 1; //videti ovo
		}

		if (SendFilePartRequest(connectSocket, partRequest) == -1) //Client unreachable, finish work here.
		{
			printf("\nClient has disconnected while trying to send file part request.");
			ShutdownConnection(connectSocket); //check if this is ok
			ReleaseSemaphore(*(outServerThreadData.EmptyOutgoingQueue), 1, NULL);
		}

		printf("\nFilePartRequest je poslat. Cekamo na odgovor Klijenta...");

		char* data = NULL;
		unsigned int length = 0;
		int partNumber = -1;

		if (RecvFilePart(connectSocket, &data, &length, &partNumber) == -1) //Client unreachable, finish work here.
		{
			printf("\nClient has disconnected while trying to recieve file part.");
			ShutdownConnection(connectSocket); //check if this is ok
			ReleaseSemaphore(*(outServerThreadData.EmptyOutgoingQueue), 1, NULL);
		}

		struct sockaddr_in socketAddress;
		int socketAddress_len = sizeof(socketAddress);

		if (getsockname(*listenSocket, (sockaddr *)&socketAddress, &socketAddress_len) == -1)
		{
			printf("getsockname() failed.\n"); 
			return -1;
		}
		
		CLIENT_FILE_PART_INFO filePartInfo;

		HandleRecievedFilePart(wholeFile, data, length, partNumber, fileParts);

		free(data);
		data = NULL;

		ReleaseSemaphore(*(outServerThreadData.EmptyOutgoingQueue), 1, NULL);
		printf("\nReleased EmptyOutgoingQueue.");
	}

	return 0;
}

DWORD WINAPI ProccessIncomingFilePartRequest(LPVOID param)
{
	INC_THREAD_DATA incThreadData = *((INC_THREAD_DATA*)param);
	Queue<SOCKET>* incomingRequestQueue = incThreadData.incomingRequestQueue;
	HashMap<SOCKET>* processingSocketsMap = incThreadData.processingSocketsMap;
	LinkedList<CLIENT_FILE_PART_INFO>* fileParts = incThreadData.fileParts;
	LinkedList<SOCKET>* acceptedSockets = incThreadData.acceptedSockets;
	int* shutDownClient = incThreadData.shutDownClient;

	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { *(incThreadData.FinishSignal), *(incThreadData.FullIncomingQueue) };
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
	{

		SOCKET requestSocket;

		int getResult = incomingRequestQueue->DequeueGet(&requestSocket);

		if (getResult == 0)
			continue;

		printf("\nTaken from IncomingRequestQueue");

		//Convert socket to string so we can use it as key
		char socketBuffer[SOCKET_BUFFER_SIZE];
		sprintf_s(socketBuffer, "%d", requestSocket);

		FILE_PART_REQUEST partRequest;

		//Recieve file part request
		int iResult = RecvFilePartRequest(requestSocket, &partRequest); 

		if (iResult == -1) //Error recieving, finish work here.
		{
			printf("\nClient has disconnected while tring to recieve file part request.");
			iResult = ShutdownConnection(requestSocket);
			iResult += RemoveSocketFromArray(acceptedSockets, requestSocket);
			if (iResult != 0) //Shutdown everything in case something went terribly wrong
			{
				ReleaseSemaphore(*(incThreadData.FinishSignal), CLIENT_THREADS, NULL);
				printf("\nRemove client failed failed");
				*shutDownClient = 0;
			}
			processingSocketsMap->Delete((const char*)(socketBuffer));
			continue;
		}

		CLIENT_FILE_PART_INFO partToSend;

		int partNumber = 0;

		//Trying to find file part
		if (FindFilePart(fileParts, &partToSend, partRequest.fileName) != 0)
		{
			printf("\nCouldn't find filePart.");
			partToSend.partBuffer = (char*)malloc(sizeof(char) * 11);
			strcpy_s(partToSend.partBuffer, 11, "FailedFind");
			partToSend.length = 11;
			partNumber = -1;
		}
		else
		{
			partNumber = partRequest.partNumber;
		}

		//Sending file part to client
		if (SendFilePart(requestSocket, partToSend.partBuffer, partToSend.length, partNumber) == -1)
		{
			printf("\nClient has disconnected while trying to send file part to him.");
			ShutdownConnection(requestSocket);
			processingSocketsMap->Delete((const char*)(socketBuffer));
		}

		free(partToSend.partBuffer);

		printf("\nPoslato s klijenta - klijentu");
		processingSocketsMap->Delete((const char*)(socketBuffer));
	}

	return 0;
}
