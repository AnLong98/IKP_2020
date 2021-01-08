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
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

using namespace std;

HANDLE finishSignal;

CRITICAL_SECTION UserInput;
CRITICAL_SECTION FileAccess;

bool InitializeWindowsSockets();
DWORD WINAPI ProcessIncomingFileParts(LPVOID param);
//void AddPartToFileBuffer(CLIENT_DOWNLOADING_FILE file, unsigned int fileSize);


SOCKET listenSocket;
CLIENT_DOWNLOADING_FILE wholeFile; //ovde delice koje dobijamo sastavljamo
list <CLIENT_FILE_PART_INFO> fileParts;
CLIENT_FILE_PART_INFO allParts[FILE_PARTS];

int main()
{
	//SOCKET connectSocket = INVALID_SOCKET;
	listenSocket = INVALID_SOCKET;
	if (InitializeWindowsSockets() == false)
	{
		return 1;
	}

	int iResult;

	fd_set readfds;
	unsigned long mode = 1; 

	// napraviti jednu nit i funkciju koja ce da radi sa serverom.
	HANDLE process1 = NULL;
	DWORD dwWaitResult;
	DWORD Process1ID;

	//InitializeCriticalSection(&FileAccess);
	InitializeCriticalSection(&UserInput);

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

	process1 = CreateThread(NULL, 0, &ProcessIncomingFileParts, (LPVOID)0, 0, &Process1ID);

	printf("Client is up.");

	//CEKAM DA SE MOJA NIT ZAVRSI
	dwWaitResult = WaitForSingleObject(
		process1,
		INFINITE);

	switch (dwWaitResult)
	{
		// All thread objects were signaled
	case WAIT_OBJECT_0:
		printf("\nThread ended, cleaning up for application exit...\n");
		break;

		// An error occurred
	default:
		printf("WaitForSingleObject failed (%d)\n", GetLastError());
		return 1;
	}
	
	getchar(); //ne zatvaraj klijenta dok nesto ne pritisnem.

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

DWORD WINAPI ProcessIncomingFileParts(LPVOID param)
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
	char filename[MAX_FILE_NAME];

	EnterCriticalSection(&UserInput);
	printf("\nEnter a file name: ");
	gets_s(file.fileName, MAX_FILE_NAME);
	LeaveCriticalSection(&UserInput);

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

	char* data = NULL;
	unsigned int length;
	int partNumber;
	unsigned int prevLength = 0;
	CLIENT_FILE_PART_INFO part;

	strcpy(wholeFile.fileName, file.fileName);
	unsigned int fileSize = response.fileSize;
	wholeFile.bufferPointer = (char*)calloc(fileSize, fileSize);

	for (int i = 0; i < (int)response.serverPartsNumber; i++)
	{
		RecvFilePart(connectSocket, &data, &length, &partNumber);

		if (partNumber == response.filePartToStore)
		{
			strcpy(part.filename, file.fileName);
			part.partBuffer = (char*)malloc(length);
			strcpy(part.partBuffer, data);
			part.lenght = length;
			fileParts.push_back(part); //ovde iskoristiti listu sto napravimo mi.
		}

		if (partNumber != 9)
		{
			memcpy(wholeFile.bufferPointer + length * partNumber, data, length);
		}
		else
		{
			memcpy(wholeFile.bufferPointer + prevLength * partNumber, data, length);
		}

		wholeFile.partsDownloaded++;

		prevLength = length;
		
		free(data);
		data = NULL;
	}

	if (wholeFile.partsDownloaded == FILE_PARTS)
	{
		WriteFileIntoMemory(wholeFile.fileName, wholeFile.bufferPointer, strlen(wholeFile.bufferPointer));

	}


}

/*void AddPartToFileBuffer(CLIENT_DOWNLOADING_FILE file, unsigned int fileSize)
{
	if(file)
}
*/