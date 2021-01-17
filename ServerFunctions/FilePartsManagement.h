#pragma once
#include <ws2tcpip.h>
#include "../ServerFunctions/ServerStructs.h"
#include "../DataStructures/HashMap.h"

/*
	Initializes library handle, and allows thread safe access. Needs to be called ONCE before use.
*/
void InitFileManagementHandle();

/*
	Deletes library handle, after calling this library will become unusable.
*/
void DeleteFileManagementHandle();

/*
	Divides file passed by fileBuffer into parts, assigns all file parts to server and allocates memory for file part array.
	It's client's responsibility to free memory after use.

	loadedFileBuffer - Pointer to file buffer
	fileSize - File size.
	parts - Number of parts to  which file will be divided.
	unallocatedFilePartArray - Pointer to pointer of file parts array, pointer should be passed as NULL and memory will be allocated by function.

	Returns 0 if successfull -1 if out of memory and -2 if library handle not initialized.
	
	THREAD SAFE
*/
int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART** unallocatedPartsArray);

/*
	Assigns a file part to the client. Expands file parts array if more memory is required.

	clientInfo - Client listen socket address.
	fileName - File name.
	fileInfoMap - Hash map which stores informations about clients.

	Returns assigned file part number -1 if out of memory and -2 if library handle not initialized.

	THREAD SAFE
*/
int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName, HashMap<FILE_DATA>* fileInfoMap);

/*
	Unassigns client owned file part. Assigns part back to server.

	clientInfo - Client listen socket address.
	fileName - File name.
	fileInfoMap - Hash map which stores informations about clients.
	fileDataArray - Array of client owned file data.
	filePartsCount - Number of client owned parts.

	Returns assigned file part number -1 if out of memory and -2 if library handle not initialized.

	THREAD SAFE
*/
int UnassignFileParts(SOCKADDR_IN clientInfo, HashMap<FILE_DATA>* fileInfoMap, FILE_DATA* fileDataArray, unsigned int filePartsCount);

/*
	Packs file response about existing loaded file. Assigns file part to client requester.

	response - Pointer to file response struct in which response will be packed
	fileData - Loaded file data
	request -  Client's file request
	serverOwnedParts - Array of server owned part indexes which will be set
	fileInfoMap - Hash map of file info

	Returns 0 if successfull, -1 if out of memory.

	THREAD SAFE

*/
int PackExistingFileResponse(FILE_RESPONSE* response, FILE_DATA fileData, FILE_REQUEST request, int* serverOwnedParts, HashMap<FILE_DATA>* fileInfoMap);