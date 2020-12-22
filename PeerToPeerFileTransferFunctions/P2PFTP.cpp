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
	iResult = send(s, (char*)htonl(length), sizeof(unsigned int), 0);
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

	// #2. Recv file part size
	iResult = recv(s, buffer, sizeof(unsigned int), 0);
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

	// #3. Recv file part 
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
	FILE_PART_REQUEST fileReq;
	memcpy(fileReq.fileName, request.fileName, 70);
	fileReq.partNumber = htonl(request.partNumber);

	// #1. Send Request
	int iResult = send(s, (char*)&fileReq, sizeof(FILE_PART_REQUEST), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	return 0;
}

int RecvFilePartRequest(SOCKET s, FILE_PART_REQUEST* request)
{
	FILE_PART_REQUEST fileReq;
	char buffer[sizeof(FILE_PART_REQUEST)];

	// #1. Receive request
	int iResult = recv(s, buffer, sizeof(FILE_PART_REQUEST), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}
	memcpy(&fileReq, buffer, sizeof(FILE_PART_REQUEST));
	memcpy(request->fileName, fileReq.fileName, 70);
	request->partNumber = ntohl(fileReq.partNumber);

	return 0;
}


int SendFileRequest(SOCKET s, FILE_REQUEST request)
{
	request.requesterListenAddress.sin_port = htons(request.requesterListenAddress.sin_port);
	int iResult = send(s, (char*)&request, sizeof(FILE_REQUEST), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	return 0;
}

int RecvFileRequest(SOCKET s, FILE_REQUEST* request)
{
	char buffer[sizeof(FILE_REQUEST)];
	int iResult = recv(s, buffer, sizeof(FILE_REQUEST), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	request->requesterListenAddress.sin_port = ntohs(request->requesterListenAddress.sin_port);

	return 0;
}


int SendFileResponse(SOCKET s, FILE_RESPONSE response)
{
	for (int i = 0; i < response.clientPartsNumber; i++)
	{
		//Potentially change this to something more meaningful
		response.clientParts[i].partNumber = htonl(response.clientParts[i].partNumber);
		response.clientParts[i].clientOwnerAddress.sin_port = htons(response.clientParts[i].clientOwnerAddress.sin_port);
	}

	response.clientPartsNumber = htonl(response.clientPartsNumber);
	response.fileExists = htonl(response.fileExists);
	response.filePartToStore = htonl(response.filePartToStore);
	response.serverPartsNumber = htonl(response.serverPartsNumber);
	// #1. Send Response
	int iResult = send(s, (char*)&response, sizeof(FILE_RESPONSE), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	return 0;
}

int RecvFileResponse(SOCKET s, FILE_RESPONSE* response)
{
	char buffer[sizeof(FILE_RESPONSE)];
	// #1. Recv Response
	int iResult = recv(s, buffer, sizeof(FILE_RESPONSE), 0);
	if (iResult == SOCKET_ERROR)
	{
		//HANDLE
		return -1;
	}

	memcpy(response, buffer, sizeof(FILE_RESPONSE));

	//convert endianess
	response->clientPartsNumber = ntohl(response->clientPartsNumber);
	response->fileExists = ntohl(response->fileExists);
	response->filePartToStore = ntohl(response->filePartToStore);
	response->serverPartsNumber = ntohl(response->serverPartsNumber);

	for (int i = 0; i < response->clientPartsNumber; i++)
	{
		response->clientParts[i].partNumber = ntohl(response->clientParts[i].partNumber);
		response->clientParts[i].clientOwnerAddress.sin_port = ntohs(response->clientParts[i].clientOwnerAddress.sin_port);
	}

	return 0;
}
