#include "../ServerFunctions/ClientInfoManagement.h"

CRITICAL_SECTION ClientInfoAccess;
int isInitClientInfoHandle = 0;

void InitClientInfoHandle()
{
	InitializeCriticalSection(&ClientInfoAccess);
	isInitClientInfoHandle = 1;
}

void DeleteClientInfoHandle()
{
	DeleteCriticalSection(&ClientInfoAccess);
	isInitClientInfoHandle = 0;
}

int AddClientInfo(SOCKET* socket, FILE_DATA data, SOCKADDR_IN clientAddress, HashMap<CLIENT_INFO>* clientInformationsMap)
{
	if(!isInitClientInfoHandle)
		return -2;
	EnterCriticalSection(&ClientInfoAccess);
	if (clientInformationsMap->DoesKeyExist((const char*)(socket))) //Client is already in info map
	{
		CLIENT_INFO info;
		clientInformationsMap->Get((const char*)(socket), &info);

		if (info.fileDataArraySize == info.ownedFilesCount)
		{
			info.clientOwnedFiles = (FILE_DATA*)realloc(info.clientOwnedFiles, info.fileDataArraySize + FILE_PARTS);
			info.fileDataArraySize *= 2;
		}
		info.clientOwnedFiles[info.ownedFilesCount] = data;
		info.ownedFilesCount++;
		clientInformationsMap->Insert((const char*)(socket), info);
		LeaveCriticalSection(&ClientInfoAccess);
		return 0;

	}

	CLIENT_INFO info;
	info.clientOwnedFiles = (FILE_DATA*)malloc(FILE_PARTS * sizeof(FILE_DATA));
	info.clientAddress = clientAddress;
	info.clientSocket = socket;
	info.fileDataArraySize = FILE_PARTS;
	info.ownedFilesCount = 1;
	info.clientOwnedFiles[0] = data;
	clientInformationsMap->Insert((const char*)(socket), info);
	LeaveCriticalSection(&ClientInfoAccess);
	return 0;

}


int RemoveClientInfo(SOCKET* clientSocket, HashMap<CLIENT_INFO>* clientInformationsMap)
{
	if (!isInitClientInfoHandle)
		return -2;

	if (clientInformationsMap->DoesKeyExist((const char*)(clientSocket))) //Client is already in info map
	{
		CLIENT_INFO info;
		clientInformationsMap->Get((const char*)(clientSocket), &info);

		free(info.clientOwnedFiles);
		clientInformationsMap->Delete((const char*)(clientSocket));
		return 0;

	}
	else {
		return -1;
	}

}