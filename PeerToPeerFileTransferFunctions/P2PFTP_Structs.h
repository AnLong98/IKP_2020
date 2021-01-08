#pragma once
#include <ws2tcpip.h>
#include "../PeerToPeerFileTransferFunctions/P2PLimitations.h"

typedef struct FILE_PART_REQUEST
{
	char fileName[MAX_FILE_NAME];
	unsigned int partNumber;
}F_PART_REQ;

typedef struct FILE_REQUEST
{
	char fileName[MAX_FILE_NAME];
	SOCKADDR_IN requesterListenAddress;
}F_REQUEST;

typedef struct FILE_PART_INFO
{
	SOCKADDR_IN clientOwnerAddress;
	unsigned int partNumber;
}F_PART_INFO;

typedef struct FILE_RESPONSE
{
	FILE_PART_INFO clientParts[MAX_CLIENTS];
	unsigned int fileExists;
	unsigned int clientPartsNumber;
	unsigned int serverPartsNumber;
	unsigned int filePartToStore;
	unsigned int fileSize;
}F_RESPONSE;


