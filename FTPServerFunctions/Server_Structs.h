#pragma once
#include <ws2tcpip.h>
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"


typedef struct FILE_PART
{
	SOCKADDR_IN clientOwnerAddress;
	unsigned int filePartNumber;
	int isServerOnly;
	unsigned int partSize;
	char* partStartPointer;
}F_PART;

typedef struct FILE_DATA
{
	char* filePointer;
	FILE_PART* filePartDataArray;
	unsigned int partArraySize;
	unsigned int partsOnClients;
	unsigned int nextPartToAssign;
}F_DATA;

typedef struct CLIENT_INFO
{
	SOCKET* clientSocket;
	FILE_DATA* clientOwnedFiles;
	unsigned int fileDataArraySize;
	unsigned int ownedFilesCount;
}C_INFO;
