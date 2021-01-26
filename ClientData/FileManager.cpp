#include "../ClientData/FileManager.h"
#include "../FileIO_Functions/FileIO.h"
#include <direct.h>


CRITICAL_SECTION FilePartAccess;
int isInitFileManagerHandle = 0;

CRITICAL_SECTION WholeFileAccess;
int isInitWholeFileManagerHandle = 0;

void InitFileAcessManagementHandle()
{
	InitializeCriticalSection(&FilePartAccess);
	isInitFileManagerHandle = 1;
}

void DeleteFileAccessManagementHandle()
{
	DeleteCriticalSection(&FilePartAccess);
	isInitFileManagerHandle = 0;
}

void InitWholeFileManagementHandle()
{
	InitializeCriticalSection(&WholeFileAccess);
	isInitWholeFileManagerHandle = 1;
}

void DeleteWholeFileManagementHandle()
{
	DeleteCriticalSection(&WholeFileAccess);
	isInitWholeFileManagerHandle = 0;
}

int InitWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile, char* fileName, FILE_RESPONSE response)
{
	if(!isInitWholeFileManagerHandle)
		return -2;

	EnterCriticalSection(&WholeFileAccess);
	memset(wholeFile, 0, sizeof(wholeFile));
	strcpy(wholeFile->fileName, fileName);

	//part to store
	wholeFile->filePartToStore = response.filePartToStore;
	wholeFile->fileSize = response.fileSize;

	unsigned int fileSize = response.fileSize;

	wholeFile->bufferPointer = (char*)calloc(fileSize, sizeof(char));
	LeaveCriticalSection(&WholeFileAccess);
	
	return 1;
}

void ResetWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile)
{
	EnterCriticalSection(&WholeFileAccess);
	free(wholeFile->bufferPointer);
	wholeFile->bufferPointer = NULL;
	wholeFile->partsDownloaded = 0;
	wholeFile->fileSize = 0;
	LeaveCriticalSection(&WholeFileAccess);
}

int HandleRecievedFilePart(CLIENT_FILE_PART_INFO* filePartInfo, CLIENT_DOWNLOADING_FILE* wholeFile, char* data, int length, int partNumber, LinkedList<CLIENT_FILE_PART_INFO>* fileParts)
{

	if (!isInitFileManagerHandle)
		return -2;

	if (!isInitWholeFileManagerHandle)
		return -2;

	bool savePart = true;

	EnterCriticalSection(&FilePartAccess);
	ListNode<CLIENT_FILE_PART_INFO>* nodeFront = fileParts->AcquireIteratorNodeFront();
	while (nodeFront != nullptr)
	{
		if (strcmp(nodeFront->GetValue().filename, wholeFile->fileName) == 0)
		{
			savePart = false;
		}
		
		nodeFront = nodeFront->Next();
	}
	LeaveCriticalSection(&FilePartAccess);

	if (partNumber == wholeFile->filePartToStore && savePart)
	{
		strcpy(filePartInfo->filename, wholeFile->fileName);
		filePartInfo->partBuffer = (char*)calloc(length, sizeof(char));
		strcpy(filePartInfo->partBuffer, data);
		filePartInfo->lenght = length;

		EnterCriticalSection(&FilePartAccess);
		fileParts->PushBack(*filePartInfo); //ovde iskoristiti listu sto napravimo mi.
		LeaveCriticalSection(&FilePartAccess);
	}

	// desice se to da ce potencijalno ova nit, i nit koja prima delove od drugih klijenata
	// u isto vreme hteti da upisu u strukturu, partsDownloaded moze biti netacno postavljen

	EnterCriticalSection(&WholeFileAccess);
	if (partNumber != 9)
	{
		memcpy(wholeFile->bufferPointer + length * partNumber, data, length);
	}
	else
	{
		memcpy(wholeFile->bufferPointer + wholeFile->fileSize - length, data, length);
	}
	wholeFile->partsDownloaded++;
	LeaveCriticalSection(&WholeFileAccess);

	return 1;
}

void WriteWholeFileIntoMemory(char* dirName, CLIENT_DOWNLOADING_FILE wholeFile)
{
	//automatski proverava da li folder postoji, ako postoji, nece ga napraviti opet
	_mkdir(dirName);
	char* filePath = (char*)malloc(sizeof(dirName) + sizeof("/") + sizeof(wholeFile.fileName));
	strcpy(filePath, dirName);
	strcat(filePath, "/");
	strcat(filePath, wholeFile.fileName);
	WriteFileIntoMemory(filePath, wholeFile.bufferPointer, strlen(wholeFile.bufferPointer));
	free(filePath);
	filePath = NULL;
}

void FindFilePart(LinkedList<CLIENT_FILE_PART_INFO>* fileParts, CLIENT_FILE_PART_INFO* partToSend, char fileName[])
{
	EnterCriticalSection(&FilePartAccess);
	ListNode<CLIENT_FILE_PART_INFO>* nodeFront = fileParts->AcquireIteratorNodeFront();
	while (nodeFront != nullptr)
	{
		if (strcmp(nodeFront->GetValue().filename, fileName) == 0)
		{
			strcpy(partToSend->filename, nodeFront->GetValue().filename);
			partToSend->lenght = nodeFront->GetValue().lenght;
			partToSend->partBuffer = (char*)malloc(sizeof(char)*partToSend->lenght);
			strcpy(partToSend->partBuffer, nodeFront->GetValue().partBuffer); // access violation
		}

		nodeFront = nodeFront->Next();
	}
	LeaveCriticalSection(&FilePartAccess);
}