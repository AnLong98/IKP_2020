#include "../PeerToPeerFileTransferFunctions/P2PFTP.h"

int SendFilePart(SOCKET s, char* data, unsigned int length, int partNumber)
{
	return 0;
}

int RecvFilePart(SOCKET s, char* data, unsigned int* length, int* partNumber)
{
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
