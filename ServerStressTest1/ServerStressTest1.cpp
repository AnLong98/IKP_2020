// ServerStressTest1.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#pragma comment(lib, "Ws2_32.lib")
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <conio.h>
#define REQUESTS_PER_THREAD 20
#define TEST_FILE_COUNT 4
#define STRES_TEST_THREADS 15
#define DEFAULT_PORT 27016
#define SERVER_ADDRESS "127.0.0.1"
#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#include "../ServerStressTest1/StressTestStucts.h"
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"
#include "../PeerToPeerFileTransferFunctions/P2PLimitations.h"
#define SAFE_DELETE_HANDLE(a)  if(a){CloseHandle(a);}

bool InitializeWindowsSockets();
DWORD WINAPI RequestSender(LPVOID params);

int main()
{
	HANDLE processors[STRES_TEST_THREADS] = { NULL };
	DWORD processorIDs[STRES_TEST_THREADS];
	int threadsWorking = 1;
	int firstPortNumber = 1;

	//Array of server test file names
	const char * testFiles[] = {
	"Test100KB.txt",
	"Test1MB.txt",
	"Test100MB.txt",
	"Test300MB.txt",
	};

	if (InitializeWindowsSockets() == false)
	{
		printf("\n Stress test failed to initalize WSAE.");
		return 1;
	}

	for (int i = 0; i < STRES_TEST_THREADS; i++)
	{
		TEST_REQUEST_SENDER_DATA* threadData =  (TEST_REQUEST_SENDER_DATA*) malloc(sizeof(TEST_REQUEST_SENDER_DATA));
		threadData->firstPortNumber = firstPortNumber;
		threadData->lastPortNumber = firstPortNumber + REQUESTS_PER_THREAD;
		threadData->testFileCount = TEST_FILE_COUNT;
		threadData->testFileNames = testFiles;
		threadData->threadsWorking = &threadsWorking;
		threadData->threadID = i;
		processors[i] = CreateThread(NULL, 0, &RequestSender, (LPVOID)threadData, 0, processorIDs + i);
		firstPortNumber += REQUESTS_PER_THREAD + 1;
	}
		

	for (int i = 0; i < STRES_TEST_THREADS; i++)
	{
		if (!processors[i]) {

			threadsWorking = 0;
			printf("\nStress test failed as threads failed to create.");

			for (int i = 0; i < STRES_TEST_THREADS; i++)
				SAFE_DELETE_HANDLE(processors[i]);

			return 0;
		}
	}
	for (int i = 0; i < STRES_TEST_THREADS; i++)
	{
		WaitForSingleObject(processors[i], INFINITE);
	}

	printf("\nStress Test 1 finished");

	for (int i = 0; i < STRES_TEST_THREADS; i++)
		SAFE_DELETE_HANDLE(processors[i]);

	WSACleanup();
	_getch();
	return 0;

}

DWORD WINAPI RequestSender(LPVOID params)
{
	TEST_REQUEST_SENDER_DATA data = *((TEST_REQUEST_SENDER_DATA*)params);
	printf("\nThread starting with %d - %d", data.firstPortNumber, data.lastPortNumber);
	for (int i = 0; i < REQUESTS_PER_THREAD; i++)
	{
		if (!(*(data.threadsWorking)))
			break;

		printf("\n-------THREAD %d is sending Request # %d------", GetCurrentThreadId(), i);

		/*CONNECT TO SERVER*/
		SOCKET connectSocket = INVALID_SOCKET;

		connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sockaddr_in serverAddress;
		serverAddress.sin_family = AF_INET;
		InetPton(AF_INET, TEXT(SERVER_ADDRESS), &serverAddress.sin_addr.s_addr);
		serverAddress.sin_port = htons(DEFAULT_PORT);

		if (connect(connectSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR)
		{
			printf("\nUnable to connect to server.");
			closesocket(connectSocket);
			return 1;
		}

		/*PREPARE REQUEST FOR SENDING*/
		struct sockaddr_in socketAddress;
		InetPton(AF_INET, TEXT("192.168.1.1"), &socketAddress.sin_addr.s_addr);
		socketAddress.sin_port = data.firstPortNumber + i;

		FILE_REQUEST file;
		file.requesterListenAddress = socketAddress;

		int fileIndex = i % (TEST_FILE_COUNT);
		strcpy_s(file.fileName, strlen(data.testFileNames[fileIndex]) + 1, data.testFileNames[fileIndex]);

		if (SendFileRequest(connectSocket, file) == -1)
		{
			closesocket(connectSocket);
			printf("\nSendFileRequest return an error.");
			return 1;
		}


		/*READ RESPONSE*/
		FILE_RESPONSE response;

		if (RecvFileResponse(connectSocket, &response) == -1)
		{
			closesocket(connectSocket);
			printf("\nRecvFileResponse return an error. %ld", WSAGetLastError());
			return 1;
		}

		/* READ SERVER PARTS FROM REPSONSE TO PREVENT SERVER BLOCKING ON SEND*/
		char* part = NULL;
		unsigned int length;
		int partNumber;
		for (int i = 0; i < (int)response.serverPartsNumber; i++)
		{
			if (RecvFilePart(connectSocket, &part, &length, &partNumber) == -1)
			{
				printf("\nCouldn't receive part %d form server. %ld", partNumber,  WSAGetLastError());
				closesocket(connectSocket);
				return 1;
			}
			free(part);
		}
		shutdown(connectSocket, SD_BOTH);
		closesocket(connectSocket);
		connectSocket = INVALID_SOCKET;
		
	}
	free((TEST_REQUEST_SENDER_DATA*)params);
	printf("\nThread finished working");
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