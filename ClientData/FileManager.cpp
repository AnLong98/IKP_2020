/*#include "../ClientData/FileManager.h"
#include <list>

using namespace std;

//pravim fju koja stavlja delic u wholefile
//zbog kriticnih sekcija

CRITICAL_SECTION FilePartAccess;
int isInitFileManagerHandle = 0;

CRITICAL_SECTION WholeFileAccess;
int isInitFileManagerHandle2 = 0;

void InitFileManagementHandle()
{
	InitializeCriticalSection(&FilePartAccess);
	isInitFileManagerHandle = 1;
}

void DeleteFileManagementHandle()
{
	DeleteCriticalSection(&FilePartAccess);
	isInitFileManagerHandle = 0;
}

void InitFileManagementHandle()
{
	InitializeCriticalSection(&WholeFileAccess);
	isInitFileManagerHandle2 = 1;
}

void DeleteFileManagementHandle()
{
	DeleteCriticalSection(&WholeFileAccess);
	isInitFileManagerHandle2 = 0;
}

int HandleRecievedFilePart(CLIENT_FILE_PART_INFO* filePartInfo, CLIENT_DOWNLOADING_FILE* wholeFile, char* data, int length, int partNumber)
{

	if (!isInitFileManagerHandle)
		return -2;

	bool savePart = true;

	EnterCriticalSection(&FilePartAccess);
	list<CLIENT_FILE_PART_INFO>::iterator it;
	for (it = fileParts.begin(); it != fileParts.end(); ++it) {
		if (strcmp(it->filename, wholeFile->fileName) == 0)
		{
			savePart = false;
		}
	}
	LeaveCriticalSection(&FilePartAccess);

	if (partNumber == wholeFile->filePartToStore && savePart)
	{
		strcpy(filePartInfo->filename, wholeFile->fileName);
		filePartInfo->partBuffer = (char*)calloc(length, sizeof(char));
		strcpy(filePartInfo->partBuffer, data);
		filePartInfo->lenght = length;

		EnterCriticalSection(&FilePartAccess);
		fileParts.push_back(*filePartInfo); //ovde iskoristiti listu sto napravimo mi.
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
}*/