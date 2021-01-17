#pragma once
#ifndef P2PFTP_H
#define P2PFTP_H

#include <ws2tcpip.h>
#include "../PeerToPeerFileTransferFunctions/P2PFTP_Structs.h"

/*
	Sends file part to client. For sockets in non blocking mode, if WSAEWOULDBLOCK occurs retransmission will be attempted after 1 second

	s - Socket to send to
	data - File part buffer
	length - File part length in bytes
	partNumber - File part number

	Returns 0  if successfull, -1 if error occured
*/
int SendFilePart(SOCKET s, char* data, unsigned int length, int partNumber);

/*
	Receives file part. Allocates file part buffer to store received data.

	s - Socket to receive from
	data - Point to pointer of unallocated file part buffer
	length - Pointer to store part length
	partNumber - Pointer to store part number

	Returns 0  if successfull, -1 if error occured
*/
int RecvFilePart(SOCKET s, char** data, unsigned int* length, int* partNumber);

/*
	Sends file part request

	s - Socket to send to
	request - File part request to send

	Returns 0  if successfull, -1 if error occured
*/
int SendFilePartRequest(SOCKET s, FILE_PART_REQUEST request);

/*
	Receives file part request

	s - Socket to receive from
	request - Pointer in which to store received request

	Returns 0  if successfull, -1 if error occured
*/
int RecvFilePartRequest(SOCKET s, FILE_PART_REQUEST* request);


/*
	Sends file request

	s - Socket to send to
	request - File request to send

	Returns 0  if successfull, -1 if error occured
*/
int SendFileRequest(SOCKET s, FILE_REQUEST request);

/*
	Receives file request

	s - Socket to receive from
	request - Pointer in which to store received request

	Returns 0  if successfull, -1 if error occured
*/
int RecvFileRequest(SOCKET s, FILE_REQUEST* request);

/*
	Sends file response

	s - Socket to send to
	request - File response to send

	Returns 0  if successfull, -1 if error occured
*/
int SendFileResponse(SOCKET s, FILE_RESPONSE response);

/*
	Receives file part response

	s - Socket to receive from
	request - Pointer in which to store received response

	Returns 0  if successfull, -1 if error occured
*/
int RecvFileResponse(SOCKET s, FILE_RESPONSE* response);

#endif

