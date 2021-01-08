#pragma once

#ifndef CLIENT_STRUCTS_H
#define CLIENT_STRUCTS_H

#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"

/*
	Structure representing file part data stored on client for other clients
*/
typedef struct CLIENT_FILE_PART_INFO
{
	char filename[MAX_FILE_NAME]; 
	char* partBuffer;
	unsigned int lenght;
}C_FILE_PART_INFO;

/*
	Structure representing information about entire file stored on client
*/
typedef struct CLIENT_DOWNLOADING_FILE
{
	char fileName[MAX_FILE_NAME];
	char* bufferPointer;
	unsigned int partsDownloaded;
}C_DOWNLOADING_PART;

#endif