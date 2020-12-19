#pragma once
#include <ws2tcpip.h>
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"

int SendFilePart(SOCKET s, char* data, unsigned int length, int partNumber);
int RecvFilePart(SOCKET s, char* data, unsigned int* length, int* partNumber);


int SendFilePartRequest(SOCKET s, FILE_PART_REQUEST request);
int RecvFilePartRequest(SOCKET s, FILE_PART_REQUEST* request);


int SendFileRequest(SOCKET s, FILE_REQUEST request);
int RecvFileRequest(SOCKET s, FILE_REQUEST* request);


int SendFileResponse(SOCKET s, FILE_RESPONSE response);
int RecvFileResponse(SOCKET s, FILE_RESPONSE* response);

