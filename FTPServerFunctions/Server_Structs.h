#pragma once
#include <ws2tcpip.h>
#include "../PeerToPeerFileTransferFunctions/P2PLimitations.h"


typedef struct FILE_PART
{
	SOCKADDR_IN clientOwnerAddress;
	unsigned int filePartNumber;
	int isServerOnly;
	unsigned int partSize;
	char* partStartPointer;
};

typedef struct FILE_DATA
{
	char* filePointer;
	FILE_PART* filePartDataArray;
	unsigned int partArraySize;
};

typedef struct CLIENT_INFO
{
	SOCKET clientSocket;
	FILE_DATA* clientOwnedFiles;
	unsigned int ownedFilesCount;
};
