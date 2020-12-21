#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#include <ws2tcpip.h>
#define BUFFER 20

int SendFilePart(SOCKET s, char* data, unsigned int length, int partNumber)
{
	// #1. Send file part number
	int iResult = send(s, (char*)htonl(partNumber), sizeof(int), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	// #2. Send file part size
	iResult = send(s, (char*)htonl(length), sizeof(int), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	// #3. Send file part 
	iResult = send(s, data, length, 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	return 0;

}

int RecvFilePart(SOCKET s, char* data, unsigned int* length, int* partNumber)
{
	char buffer[BUFFER];
	// #1. Recv file part number
	int iResult = recv(s, buffer, sizeof(int), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}
	*partNumber = ntohl(atoi(buffer));

	// #2. Send file part size
	int iResult = recv(s, buffer, sizeof(int), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}
	*length = ntohl(atoi(buffer));

	//Allocate memory for filePart
	data = (char*)malloc(*length);
	if (data == NULL)
	{
		//HANDLE OUT OF MEMORY
		return -1;
	}

	// #3. Send file part 
	iResult = recv(s, data, *length, 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	return 0;
}


int SendFilePartRequest(SOCKET s, FILE_PART_REQUEST request)
{
	return 0;
}

int RecvFilePartRequest(SOCKET s, FILE_PART_REQUEST* request)
{
	return 0;
}


int SendFileRequest(SOCKET s, FILE_REQUEST request)
{
	return 0;
}

int RecvFileRequest(SOCKET s, FILE_REQUEST* request)
{
	return 0;
}


int SendFileResponse(SOCKET s, FILE_RESPONSE response)
{
	return 0;
}

int RecvFileResponse(SOCKET s, FILE_RESPONSE* response)
{
	return 0;
}
