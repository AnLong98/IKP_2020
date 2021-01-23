#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"
#define BUFFER 20

int SendFilePart(SOCKET s, char* data, unsigned int length, int partNumber)
{
	char buffer[BUFFER];
	int iResult;
	while (true)
	{
		
		_itoa_s(ntohl(partNumber), buffer, 10);
		// #1. Send file part number
		iResult = send(s, buffer, BUFFER, 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			return -1;
		}
		break;
	}

	while (true)
	{
		// #2. Send file part size
		_itoa_s(ntohl(length), buffer, 10);
		iResult = send(s, buffer, BUFFER, 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}

	int bytesSent = 0;
	// #3. Send file part 
	do {
		iResult = send(s, data + bytesSent, length - bytesSent, 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		bytesSent += iResult;
	} while (bytesSent < (int)length);

	return 0;
	

}

int RecvFilePart(SOCKET s, char** data, unsigned int* length, int* partNumber)
{
	int iResult;
	char buffer[BUFFER];

	// #1. Recv file part number
	while (true)
	{
		iResult = recv(s, buffer, BUFFER, 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}
	*partNumber = ntohl(atoi(buffer));

	// #2. Recv file part size
	while (true)
	{
		iResult = recv(s, buffer, BUFFER, 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}
	*length = ntohl(atoi(buffer));

	//Allocate memory for filePart
	*data = (char*)malloc(*length);
	if (*data == NULL)
	{
		//HANDLE OUT OF MEMORY
		return -1;
	}

	// #3. Recv file part 
	int bytesReceived = 0;
	do
	{
		iResult = recv(s, *data + bytesReceived, *length - bytesReceived, 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			free(*data);
			return -1;
		}
		bytesReceived += iResult;
	} while (bytesReceived < (int)(*length));

	return 0;
}


int SendFilePartRequest(SOCKET s, FILE_PART_REQUEST request)
{
	FILE_PART_REQUEST fileReq;
	memcpy(fileReq.fileName, request.fileName, 70);
	fileReq.partNumber = htonl(request.partNumber);

	// #1. Send Request
	while (true)
	{
		int iResult = send(s, (char*)&fileReq, sizeof(FILE_PART_REQUEST), 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}

	return 0;
}

int RecvFilePartRequest(SOCKET s, FILE_PART_REQUEST* request)
{
	FILE_PART_REQUEST fileReq;
	char buffer[sizeof(FILE_PART_REQUEST)];

	// #1. Receive request
	while (true)
	{

		int iResult = recv(s, buffer, sizeof(FILE_PART_REQUEST), 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}
	memcpy(&fileReq, buffer, sizeof(FILE_PART_REQUEST));
	memcpy(request->fileName, fileReq.fileName, 70);
	request->partNumber = ntohl(fileReq.partNumber);

	return 0;
}


int SendFileRequest(SOCKET s, FILE_REQUEST request)
{
	request.requesterListenAddress.sin_port = htons(request.requesterListenAddress.sin_port);
	while (true)
	{
		int iResult = send(s, (char*)&request, sizeof(FILE_REQUEST), 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}

	return 0;
}

int RecvFileRequest(SOCKET s, FILE_REQUEST* request)
{
	char buffer[sizeof(FILE_REQUEST)];
	while (true)
	{
		int iResult = recv(s, buffer, sizeof(FILE_REQUEST), 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}
	memcpy(request, buffer, sizeof(FILE_REQUEST));
	request->requesterListenAddress.sin_port = ntohs(request->requesterListenAddress.sin_port);

	return 0;
}


int SendFileResponse(SOCKET s, FILE_RESPONSE response)
{
	for (int i = 0; i < (int)response.clientPartsNumber; i++)
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
	while (true)
	{
		int iResult = send(s, (char*)&response, sizeof(FILE_RESPONSE), 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}

	return 0;
}

int RecvFileResponse(SOCKET s, FILE_RESPONSE* response)
{
	char buffer[sizeof(FILE_RESPONSE)];
	// #1. Recv Response
	while (true)
	{
		int iResult = recv(s, buffer, sizeof(FILE_RESPONSE), 0);
		if (iResult == SOCKET_ERROR || iResult == 0)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				Sleep(1);
				continue;
			}
			//HANDLE
			return -1;
		}
		break;
	}

	memcpy(response, buffer, sizeof(FILE_RESPONSE));

	//convert endianess
	response->clientPartsNumber = ntohl(response->clientPartsNumber);
	response->fileExists = ntohl(response->fileExists);
	response->filePartToStore = ntohl(response->filePartToStore);
	response->serverPartsNumber = ntohl(response->serverPartsNumber);

	for (int i = 0; i < (int)response->clientPartsNumber; i++)
	{
		response->clientParts[i].partNumber = ntohl(response->clientParts[i].partNumber);
		response->clientParts[i].clientOwnerAddress.sin_port = ntohs(response->clientParts[i].clientOwnerAddress.sin_port);
	}

	return 0;
}
