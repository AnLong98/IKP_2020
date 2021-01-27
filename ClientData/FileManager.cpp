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

void ClearFilePartsLinkedList(LinkedList<CLIENT_FILE_PART_INFO>* fileParts)
{
	EnterCriticalSection(&FilePartAccess);
	ListNode<CLIENT_FILE_PART_INFO>* nodeFront = fileParts->AcquireIteratorNodeFront();
	while (nodeFront != nullptr)
	{
		free(nodeFront->GetValue().partBuffer);
		nodeFront = nodeFront->Next();
	}
	LeaveCriticalSection(&FilePartAccess);
}

int InitWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile, char* fileName, FILE_RESPONSE response)
{
	if(!isInitWholeFileManagerHandle)
		return -2;

	EnterCriticalSection(&WholeFileAccess);
	memset(wholeFile, 0, sizeof(wholeFile));
	strcpy(wholeFile->fileName, fileName);

	wholeFile->filePartToStore = response.filePartToStore;
	wholeFile->fileSize = response.fileSize;
	wholeFile->partsDownloaded = 0;

	unsigned int fileSize = response.fileSize;

	wholeFile->bufferPointer = (char*)malloc((fileSize + 1) * sizeof(char));
	LeaveCriticalSection(&WholeFileAccess);
	
	return 0;
}

void ResetWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile)
{
	free(wholeFile->bufferPointer);
	wholeFile->bufferPointer = NULL;
	wholeFile->partsDownloaded = 0;
	wholeFile->fileSize = 0;
}

int HandleRecievedFilePart(CLIENT_DOWNLOADING_FILE* wholeFile, char* data, int length, int partNumber, LinkedList<CLIENT_FILE_PART_INFO>* fileParts)
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

	CLIENT_FILE_PART_INFO filePartInfo;

	if (partNumber == wholeFile->filePartToStore && savePart)
	{
		strcpy(filePartInfo.filename, wholeFile->fileName);
		filePartInfo.partBuffer = (char*)malloc((length + 1) * sizeof(char));
		strcpy_s(filePartInfo.partBuffer, length + 1, data);
		filePartInfo.length = length;

		EnterCriticalSection(&FilePartAccess);
		fileParts->PushBack(filePartInfo);
		LeaveCriticalSection(&FilePartAccess);
	}

	EnterCriticalSection(&WholeFileAccess);
	if (partNumber != 9)
	{
		memcpy(wholeFile->bufferPointer + length * partNumber, data, length);
	}
	else
	{
		memcpy(wholeFile->bufferPointer + wholeFile->fileSize - length, data, length);
		wholeFile->bufferPointer[wholeFile->fileSize] = '\0';
	}
	(wholeFile->partsDownloaded)++;
	LeaveCriticalSection(&WholeFileAccess);

	return 0;
}

int WriteWholeFileIntoMemory(char* dirName, CLIENT_DOWNLOADING_FILE wholeFile)
{
	_mkdir(dirName);
	char* filePath = (char*)malloc(strlen(dirName) + strlen("/") + strlen(wholeFile.fileName) + 3); //ne sizeof.
	strcpy_s(filePath, strlen(dirName) + 1, dirName);
	strcat(filePath, "/");
	strcat(filePath, wholeFile.fileName);

	if (WriteFileIntoMemory(filePath, wholeFile.bufferPointer, wholeFile.fileSize) != 0)
	{
		free(filePath);
		filePath = NULL;
		return -1;
	}
	
	free(filePath);
	filePath = NULL;

	return 0;
}

int FindFilePart(LinkedList<CLIENT_FILE_PART_INFO>* fileParts, CLIENT_FILE_PART_INFO* partToSend, char fileName[])
{
	if (!isInitFileManagerHandle)
		return -2;

	EnterCriticalSection(&FilePartAccess);
	ListNode<CLIENT_FILE_PART_INFO>* nodeFront = fileParts->AcquireIteratorNodeFront();
	while (nodeFront != nullptr)
	{
		if (strcmp(nodeFront->GetValue().filename, fileName) == 0)
		{
			strcpy(partToSend->filename, nodeFront->GetValue().filename);
			partToSend->length = nodeFront->GetValue().length;
			partToSend->partBuffer = (char*)malloc(sizeof(char)*(partToSend->length + 1));
			strcpy_s(partToSend->partBuffer,partToSend->length + 1 , nodeFront->GetValue().partBuffer); // access violation
			LeaveCriticalSection(&FilePartAccess);
			return 0;
		}

		nodeFront = nodeFront->Next();
	}
	LeaveCriticalSection(&FilePartAccess);
	return -1;
}