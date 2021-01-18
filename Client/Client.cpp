// Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#pragma comment(lib, "Ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <ws2tcpip.h>
#include <winsock2.h>
#include <stdlib.h>
#include <time.h>
#include <queue>
#include <unordered_map>
#include <list> 
#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"
#include "../PeerToPeerFileTransferFunctions/P2PLimitations.h"
#include "../FileIO_Functions/FileIO.h"
#include "../ClientData/Client_Structs.h"
#define DEFAULT_PORT 27016
#define SERVER_ADDRESS "127.0.0.1"
#define MAX_QUEUE 20
#define CLIENT_THREADS 5
#define OUT_QUEUE_THREADS 3
#define INC_QUEUE_THREADS 2
#define SERVER_THREAD 1
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

using namespace std;

bool InitializeWindowsSockets();
DWORD WINAPI ProcessConnectionToServer(LPVOID param);
DWORD WINAPI ProcessOutgoingFilePartRequest(LPVOID param);
DWORD WINAPI ProccessIncomingFilePartRequest(LPVOID param);
int ShutdownConnection(SOCKET* socket);
int AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket);
int RemoveSocketFromArray(SOCKET acceptedSockets[], SOCKET* socket, int *freeIndex);
int CheckSetSockets(int* socketsTaken, SOCKET acceptedSockets[], fd_set* readfds);


HANDLE EmptyOutgoingQueue;
HANDLE FullOutgoingQueue;
HANDLE EmptyIncomingQueue;
HANDLE FullIncomingQueue;
HANDLE FinishSignal;

CRITICAL_SECTION OutQueueAccess;
CRITICAL_SECTION IncQueueAccess;
CRITICAL_SECTION WholeFileAccess;
CRITICAL_SECTION AcceptedSocketsAccess;
CRITICAL_SECTION ProcessingSocketsAccess; 
CRITICAL_SECTION FilePartsAccess;


queue<CLIENT_FILE_PART> outgoingRequestQueue;
queue<SOCKET*> incomingRequestQueue;
SOCKET listenSocket;
CLIENT_DOWNLOADING_FILE wholeFile; //ovde delice koje dobijamo sastavljamo
list <CLIENT_FILE_PART_INFO> fileParts; //delici za druge klijente
SOCKET acceptedSockets[MAX_CLIENTS];
int processingSockets[MAX_CLIENTS];
int socketsTaken = 0;

int main()
{

	//SOCKET connectSocket = INVALID_SOCKET;
	listenSocket = INVALID_SOCKET;

	int iResult;

	fd_set readfds;
	unsigned long mode = 1;

	HANDLE servProc = NULL;
	DWORD servProcID;
	DWORD dwWaitResult;

	HANDLE processors[CLIENT_THREADS] = { NULL };
	DWORD processorIDs[CLIENT_THREADS];

	//stavljam da tri niti rade sa OutReqQueue
	EmptyOutgoingQueue = CreateSemaphore(0, OUT_QUEUE_THREADS, OUT_QUEUE_THREADS, NULL);
	FullOutgoingQueue = CreateSemaphore(0, 0, OUT_QUEUE_THREADS, NULL);

	//stavljam da dve niti rade sa IncReqQueue
	EmptyIncomingQueue = CreateSemaphore(0, INC_QUEUE_THREADS, INC_QUEUE_THREADS, NULL);
	FullIncomingQueue = CreateSemaphore(0, 0, INC_QUEUE_THREADS, NULL);

	FinishSignal = CreateSemaphore(0, 0, CLIENT_THREADS + SERVER_THREAD, NULL);

	if (EmptyOutgoingQueue && FullOutgoingQueue && EmptyIncomingQueue && FullIncomingQueue && FinishSignal)
	{
		InitializeCriticalSection(&OutQueueAccess);
		InitializeCriticalSection(&IncQueueAccess);
		InitializeCriticalSection(&WholeFileAccess);
		InitializeCriticalSection(&FilePartsAccess);
		InitializeCriticalSection(&AcceptedSocketsAccess);
		InitializeCriticalSection(&ProcessingSocketsAccess);
		
		for (int i = 0; i < OUT_QUEUE_THREADS; i++)
		{
			processors[i] = CreateThread(NULL, 0, &ProcessOutgoingFilePartRequest, (LPVOID)0, 0, processorIDs + i);
		}

		for (int i = OUT_QUEUE_THREADS; i < CLIENT_THREADS; i++)
		{
			processors[i] = CreateThread(NULL, 0, &ProccessIncomingFilePartRequest, (LPVOID)0, 0, processorIDs + i);
		}

		for (int i = 0; i < CLIENT_THREADS; i++)
		{
			if (!processors[i])
			{
				//sta ovde raditi, ovde nigde ne pravim thread za komunikaciju sa serverom, to ide tek kasnije
				ReleaseSemaphore(FinishSignal, CLIENT_THREADS + SERVER_THREAD, NULL);
				printf("\nReleased FinishSignal");

				for (int i = 0; i < CLIENT_THREADS; i++)
					SAFE_DELETE_HANDLE(processors[i]);

				SAFE_DELETE_HANDLE(EmptyOutgoingQueue);
				SAFE_DELETE_HANDLE(FullOutgoingQueue);
				SAFE_DELETE_HANDLE(EmptyIncomingQueue);
				SAFE_DELETE_HANDLE(FullIncomingQueue);
				SAFE_DELETE_HANDLE(FinishSignal);

				DeleteCriticalSection(&OutQueueAccess);
				DeleteCriticalSection(&IncQueueAccess);
				DeleteCriticalSection(&WholeFileAccess);
				DeleteCriticalSection(&FilePartsAccess);
				DeleteCriticalSection(&AcceptedSocketsAccess);
				DeleteCriticalSection(&ProcessingSocketsAccess);

				return 0;
			}
		}
	}

	

	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	//LISTEN SOCKET ZA KLIJENTE
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
	printf("\n\nCLIENT PORT IS : ------------ %d -------------\n", socketAddress.sin_port);

	// URADITI ISTO KAO I GORE, SAMO JOS OVU NIT UGASITI
	servProc = CreateThread(NULL, 0, &ProcessConnectionToServer, (LPVOID)0, 0, &servProcID);
	

	while (true)
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

		int result = select(0, &readfds, NULL, NULL, &timeVal); 

		if (result == 0) {
			continue;
		}
		else if (result == SOCKET_ERROR) {

			//TODO: Handle this error, check which socket cause error and see if it is in sockets to close list


			printf("Errorcina je %d ", WSAGetLastError());
			ReleaseSemaphore(FinishSignal, CLIENT_THREADS + SERVER_THREAD, NULL);
			break;
		}
		else {
			if (FD_ISSET(listenSocket, &readfds) && socketsTaken < MAX_CLIENTS)
			{
				printf("\nusao sam u acceptede");
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

	for (int i = 0; i < CLIENT_THREADS; i++)
		SAFE_DELETE_HANDLE(processors[i]);
	SAFE_DELETE_HANDLE(servProc);

	SAFE_DELETE_HANDLE(EmptyOutgoingQueue);
	SAFE_DELETE_HANDLE(FullOutgoingQueue);
	SAFE_DELETE_HANDLE(EmptyIncomingQueue);
	SAFE_DELETE_HANDLE(FullIncomingQueue);
	SAFE_DELETE_HANDLE(FinishSignal);

	DeleteCriticalSection(&OutQueueAccess);
	DeleteCriticalSection(&IncQueueAccess);
	DeleteCriticalSection(&WholeFileAccess);
	DeleteCriticalSection(&FilePartsAccess);
	DeleteCriticalSection(&AcceptedSocketsAccess);
	DeleteCriticalSection(&ProcessingSocketsAccess);

	for (int i = 0; i < socketsTaken; i++)
	{
		ShutdownConnection(acceptedSockets + i);
	}
	
	// cleanup
	closesocket(listenSocket);
	WSACleanup();

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

DWORD WINAPI ProcessConnectionToServer(LPVOID param)
{
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

	if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
	{
		printf("Unable to connect to server.\n");
		closesocket(connectSocket);
		WSACleanup();
		return 1;
	}

	struct sockaddr_in socketAddress;
	int socketAddress_len = sizeof(socketAddress);

	if (getsockname(listenSocket, (sockaddr *)&socketAddress, &socketAddress_len) == -1)
	{
		printf("getsockname() failed.\n"); return -1;
	}

	FILE_REQUEST file;
	char filename[MAX_FILE_NAME] = { 0 };

	printf("\nEnter a file name: ");
	gets_s(file.fileName, MAX_FILE_NAME);

	InetPton(AF_INET, TEXT(SERVER_ADDRESS), &socketAddress.sin_addr.s_addr);

	file.requesterListenAddress = socketAddress;

	if (SendFileRequest(connectSocket, file) == -1)
	{
		closesocket(connectSocket);
		printf("SendFileRequest return an error.");
	}

	printf("\nFiles sent to server. Waiting for response...");

	FILE_RESPONSE response;

	if (RecvFileResponse(connectSocket, &response) == -1)
	{
		closesocket(connectSocket);
		printf("RecvFileResponse return an error.");
	}

	printf("\nResponse recieved from server. Waiting for parts from server...");


	strcpy(wholeFile.fileName, file.fileName);

	//part to store
	wholeFile.filePartToStore = response.filePartToStore;
	wholeFile.fileSize = response.fileSize;

	unsigned int fileSize = response.fileSize;
	//ovaj deo mi je kritican, mislim da ce raditi ovako, ali nisam siguran.
	wholeFile.bufferPointer = (char*)calloc(fileSize, sizeof(char));


	CLIENT_FILE_PART clientFilePart;

	//cim smo primili response od servera, stavljamo sve na outReqQueue
	// 
	for (int i = 0; i < (int)response.clientPartsNumber; i++)
	{
		const int semaphoreNum = 2;
		HANDLE semaphores[semaphoreNum] = { FinishSignal, EmptyOutgoingQueue };
		DWORD waitResult = WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE);

		if(waitResult == WAIT_OBJECT_0 + 1)
		{
			printf("\nEmptyOutQueue taken");

			// ulazim u kriticnu sekciju, zakljucavam outReqQueue i stavljam info u njega
			EnterCriticalSection(&OutQueueAccess);
			memcpy(clientFilePart.fileName, file.fileName, MAX_FILE_NAME);
			printf("Klijent sa portom %u ima deo %d", response.clientParts[i].clientOwnerAddress.sin_port, response.clientParts[i].partNumber);
			clientFilePart.filePartInfo.clientOwnerAddress = response.clientParts[i].clientOwnerAddress;
			clientFilePart.filePartInfo.partNumber = response.clientParts[i].partNumber;
			outgoingRequestQueue.push(clientFilePart);
			LeaveCriticalSection(&OutQueueAccess);

			ReleaseSemaphore(FullOutgoingQueue, 1, NULL);
			printf("\nFullOutQueue released");
		}

	}
	
	//delic koji cuvam za ostale klijente, stavljam ga u listu
	CLIENT_FILE_PART_INFO part;
	char* data = NULL;
	unsigned int length;
	int partNumber;

	// DRUGI KLIJENT NIKADA MI NE DODJE OVDE. IZGLEDA DA UPADNE U DEADLOCK SA SEMAFORIMA.
	// primam sada delove koje server ima kod sebe
	for (int i = 0; i < (int)response.serverPartsNumber; i++)
	{
		if (RecvFilePart(connectSocket, &data, &length, &partNumber) == -1)
		{
			printf("\nNe moze se RecvFilePart sa servera. %ld", WSAGetLastError());
			closesocket(connectSocket);
		}

		//upisujemo u listu delice koje treba da cuvamo, to isto moze da rade i druge niti
		
		if (partNumber == wholeFile.filePartToStore)
		{
			EnterCriticalSection(&FilePartsAccess);
			strcpy(part.filename, file.fileName);
			part.partBuffer = (char*)malloc(length);
			strcpy(part.partBuffer, data);
			part.lenght = length;
			fileParts.push_back(part); //ovde iskoristiti listu sto napravimo mi.
			LeaveCriticalSection(&FilePartsAccess);
		}

		// desice se to da ce potencijalno ova nit, i nit koja prima delove od drugih klijenata
		// u isto vreme hteti da upisu u strukturu, partsDownloaded moze biti netacno postavljen

		EnterCriticalSection(&WholeFileAccess);
		if (partNumber != 9)
		{
			memcpy(wholeFile.bufferPointer + length * partNumber, data, length);
		}
		else
		{
			memcpy(wholeFile.bufferPointer + wholeFile.fileSize - length, data, length);
		}
		wholeFile.partsDownloaded++;
		LeaveCriticalSection(&WholeFileAccess);

		free(data);
		data = NULL;
	}

	//proveravam da li mi je mozda server poslao sve delice
	if (wholeFile.partsDownloaded == FILE_PARTS)
	{
		WriteFileIntoMemory(wholeFile.fileName, wholeFile.bufferPointer, strlen(wholeFile.bufferPointer));
	}

	printf("\nNit za komunikaciju sa serverom je odradila svoj deo posla...");

	return 0;
}

DWORD WINAPI ProcessOutgoingFilePartRequest(LPVOID param)
{
	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { FinishSignal, FullOutgoingQueue };
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
	{
		printf("\nEntered in FullOutgoingRequestQueue");

		//zakljucavam OutReqQueue i uzimam file part
		EnterCriticalSection(&OutQueueAccess);
		CLIENT_FILE_PART filePart = outgoingRequestQueue.front();
		outgoingRequestQueue.pop();
		LeaveCriticalSection(&OutQueueAccess);


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

		if (connect(connectSocket, (SOCKADDR*)&clientAddress, sizeof(clientAddress)) == SOCKET_ERROR)
		{
			printf("Unable to connect to client.\n");
			closesocket(connectSocket);
			WSACleanup();
			return 1;
		}

		if (SendFilePartRequest(connectSocket, partRequest) == -1)
		{
			printf("\nGreska prilikom slanja zahteva za delic od klijenta.");
			closesocket(connectSocket);
		}

		printf("\nFilePartRequest je poslat. Cekamo na odgovor Klijenta...");

		char* data = NULL;
		unsigned int length;
		int partNumber;

		if (RecvFilePart(connectSocket, &data, &length, &partNumber) == -1)
		{
			printf("\nNeuspelo primanje delica od klijenta.");
			closesocket(connectSocket);
		}

		//pravimo delic koji treba da sacuvamo u listi da bi slali drugim klijentima
		CLIENT_FILE_PART_INFO filePartInfo;

		if (partNumber == wholeFile.filePartToStore)
		{
			EnterCriticalSection(&FilePartsAccess);
			strcpy(filePartInfo.filename, wholeFile.fileName);
			filePartInfo.partBuffer = (char*)malloc(length);
			strcpy(filePartInfo.partBuffer, data);
			filePartInfo.lenght = length;
			fileParts.push_back(filePartInfo); //ovde iskoristiti listu sto napravimo mi.
			LeaveCriticalSection(&FilePartsAccess);
		}
		

		// desice se to da ce potencijalno ova nit, i nit koja prima delove od drugih klijenata
		// u isto vreme hteti da upisu u strukturu, partsDownloaded moze biti netacno postavljen
		EnterCriticalSection(&WholeFileAccess);
		if (partNumber != 9)
		{
			memcpy(wholeFile.bufferPointer + length * partNumber, data, length);
		}
		else
		{
			memcpy(wholeFile.bufferPointer + wholeFile.fileSize - length, data, length);
		}
		wholeFile.partsDownloaded++;
		LeaveCriticalSection(&WholeFileAccess);

		if (wholeFile.partsDownloaded == FILE_PARTS)
		{
			WriteFileIntoMemory(wholeFile.fileName, wholeFile.bufferPointer, strlen(wholeFile.bufferPointer));
			printf("\nKlijent je primio poslednji deo i ispisao je sve u fajl !");
			//whaaaaat ??!?!?!?!?!
			//ReleaseSemaphore(FinishSignal, CLIENT_THREADS + SERVER_THREAD, NULL);
		}

		ReleaseSemaphore(EmptyOutgoingQueue, 1, NULL);
		printf("\nReleased EmptyOutgoingQueue.");

	}

	return 0;
}

DWORD WINAPI ProccessIncomingFilePartRequest(LPVOID param)
{
	const int semaphoreNum = 2;
	HANDLE semaphores[semaphoreNum] = { FinishSignal, FullIncomingQueue };
	while (WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
	{
		printf("\nZauzet FullIncomingQueue");
		EnterCriticalSection(&IncQueueAccess);
		SOCKET* requestSocket = incomingRequestQueue.front();
		incomingRequestQueue.pop();
		LeaveCriticalSection(&IncQueueAccess);

		FILE_PART_REQUEST partRequest;

		int iResult = RecvFilePartRequest(*requestSocket, &partRequest);
		if (iResult == -1)
		{
			printf("\nGreska u FullIncQueue prilikom primanja delica od klijenta.");
			//HANDLOVATI STA SVE TREBA - VIDETI SA DJAPETOM
			closesocket(*requestSocket);
		}

		char* data = NULL;
		unsigned int length;
		int partNumber;

		CLIENT_FILE_PART_INFO partToSend;

		EnterCriticalSection(&FilePartsAccess);
		list<CLIENT_FILE_PART_INFO>::iterator it;
		for (it = fileParts.begin(); it != fileParts.end(); ++it) {
			if (strcmp(it->filename, partRequest.fileName) == 0)
			{
				
				partToSend = *it;
			}
		}
		LeaveCriticalSection(&FilePartsAccess);

		if (SendFilePart(*requestSocket, partToSend.partBuffer, partToSend.lenght, partRequest.partNumber) == -1)
		{
			printf("\nGreska u FullIncQueue prilikom slanja dela sa klijenta - klijentu.");
			//handlovati ovo, videti sa djapetom sta sve
			closesocket(*requestSocket);
		}

		printf("\nPoslato s klijenta - klijentu");
		ReleaseSemaphore(EmptyIncomingQueue, 1, NULL);
		printf("\nIz FullIncQueue otpustanje EmptyIncomingQueue-a");
	}

	return 0;
}

//ovo staviti svuda gde treba da se shutdown-uje.
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

int CheckSetSockets(int* socketsTaken, SOCKET acceptedSockets[], fd_set* readfds)
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
			HANDLE semaphores[semaphoreNum] = { FinishSignal, EmptyIncomingQueue };
			//Ovde se zakuca ako iskljucim par klijenata
			DWORD waitResult = WaitForMultipleObjects(semaphoreNum, semaphores, FALSE, INFINITE);

			if (waitResult == WAIT_OBJECT_0 + 1)
			{
				printf("\nUzet EmptyIncomingQueue");

				EnterCriticalSection(&IncQueueAccess);
				printf("\nSoket %d primio", i);
				incomingRequestQueue.push(acceptedSockets + i);
				LeaveCriticalSection(&IncQueueAccess);

				ReleaseSemaphore(FullIncomingQueue, 1, NULL);

				printf("\nIz EmptyIncQueue otpusten FullIncQueue");

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