#pragma once
#pragma warning(disable:4996)
#include "../ClientData/Client_Structs.h"
#include "../DataStructures/LinkedList.h"

/*
	Initializes library handle, and allows thread safe access. Needs to be called ONCE before use.
*/
void InitFileAcessManagementHandle();

/*
	Deletes library handle, after calling this library will become unusable.
*/
void DeleteFileAccessManagementHandle();

/*
	Initializes library handle, and allows thread safe access. Needs to be called ONCE before use.
*/
void InitWholeFileManagementHandle();

/*
	Deletes library handle, after calling this library will become unusable.
*/
void DeleteWholeFileManagementHandle();

/*
	Initializes CLIENT_DOWNLOADING_FILE struct, which is used to assemble file parts into one.


	wholeFile - Pointer to CLIENT_DOWNLOADING_FILE struct in which file parts will be assembled
	fileName - Requested file name
	response - Server's file response


	Returns 0 if successfull, -2 if library handle is not initialized.
	THREAD SAFE
*/
int InitWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile, char* fileName, FILE_RESPONSE response);

/*
	Reset CLIENT_DOWNLOADING_FILE struct, so it can be reused for a new file request.


	wholeFile - Pointer to CLIENT_DOWNLOADING_FILE struct in which file parts will be assembled
	

	Does not have a return value.
	THREAD UNSAFE
*/
void ResetWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile);

/*
	Handles recieved file part. Stores marked part on Client. Assembling parts into one.


	wholeFile - Pointer to CLIENT_DOWNLOADING_FILE struct in which file parts will be assembled
	data - Data buffer recieved from Server
	length - Length of data buffer
	partNumber - Marks if part should be stored on Client
	fileParts - Pointer to LinkedList where all parts, that Client need to save, are stored


	Returns 0 if successfull, -2 if library handles are not initialized.
	THREAD SAFE
*/
int HandleRecievedFilePart(CLIENT_DOWNLOADING_FILE* wholeFile, char* data, int length, int partNumber, LinkedList<CLIENT_FILE_PART_INFO>* fileParts);

/*
	Finds file part that Client need to provide to other Client.


	fileParts - Pointer to LinkedList where all parts, that Client need to save, are stored
	partToSend - Pointer to file part which is requested
	fileName - Name of file that file part is from


	Returns 0  if successfull, -1 if file part is not found, -2 if library handles are not initialized.
	THREAD SAFE
*/
int FindFilePart(LinkedList<CLIENT_FILE_PART_INFO>* fileParts, CLIENT_FILE_PART_INFO* partToSend, char fileName[]);

/*
	Creating directory in which the file is stored. Writing whole file in memory.


	dirName - Name of directory in which file is stored.
	wholeFile - Assembled file that needs to be stored permanently.

	Returns 0 if successfull, -1 if file is not written successfully.
	THREAD UNSAFE
*/
int WriteWholeFileIntoMemory(char* dirName, CLIENT_DOWNLOADING_FILE wholeFile);

