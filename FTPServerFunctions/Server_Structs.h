#pragma once
#include <ws2tcpip.h>
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"

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
}F_DATA;

/*
	Structure representing informations about client stored on server
*/
typedef struct CLIENT_INFO
{
	SOCKET* clientSocket;
	SOCKADDR_IN clientAddress;
	FILE_DATA* clientOwnedFiles;
	unsigned int fileDataArraySize;
	unsigned int ownedFilesCount;
}C_INFO;
