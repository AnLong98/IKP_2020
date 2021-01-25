#pragma once
#include <ws2tcpip.h>
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"
#include "../DataStructures/Queue.h"
#include "../DataStructures/HashMap.h"
#include "../DataStructures/LinkedList.h"

/*
	Structure representing file part data stored on server
*/
typedef struct FILE_PART
{
	SOCKADDR_IN clientOwnerAddress;
	unsigned int filePartNumber;
	int isServerOnly;
	unsigned int partSize;
	char* partStartPointer;
}F_PART;

/*
	Structure representing informations about entire file stored on server
*/
typedef struct FILE_DATA
{
	char* filePointer;
	char fileName[MAX_FILE_NAME];
	FILE_PART* filePartDataArray;
	unsigned int partArraySize;
	unsigned int partsOnClients;
	unsigned int nextPartToAssign;
	unsigned int fileSize;
}F_DATA;

/*
	Structure representing informations about client stored on server
*/
typedef struct CLIENT_INFO
{
	SOCKET clientSocket;
	SOCKADDR_IN clientAddress;
	char** clientOwnedFiles;
	unsigned int fileDataArraySize;
	unsigned int ownedFilesCount;
}C_INFO;


/*
	Structure representing informations that server threads use
*/
typedef struct SERVER_THREAD_DATA
{
	HashMap<SOCKET> * processingSocketsMap;
	HashMap<int>* loadingFilesMap;
	HashMap<FILE_DATA> * fileInfoMap;
	Queue<SOCKET> * incomingRequestsQueue;
	HashMap<CLIENT_INFO>* clientInformationsMap;
	LinkedList<SOCKET>* acceptedSocketsArray;
	HANDLE* FinishSignal;
	HANDLE* FullQueue;
	CRITICAL_SECTION* LoadingFilesAccess;
	int* serverWorking;
}S_DATA;


/*
	Structure representing informations that server controller thread uses
*/
typedef struct SERVER_CONTROLLER_THREAD_DATA
{
	HANDLE* FinishSignal;
	int* serverWorking;
}S_CONT_DATA;
