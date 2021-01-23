#include "../ServerFunctions/ClientInfoManagement.h"
#define SOCKET_BUFFER_SIZE 20

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

int AddClientInfo(SOCKET socket, FILE_DATA data, SOCKADDR_IN clientAddress, HashMap<CLIENT_INFO>* clientInformationsMap)
{
	if(!isInitClientInfoHandle)
		return -2;
	//Convert socket to string to use as key
	char socketBuffer[SOCKET_BUFFER_SIZE];
	sprintf_s(socketBuffer, "%d", socket);

	EnterCriticalSection(&ClientInfoAccess);
	if (clientInformationsMap->DoesKeyExist((const char*)(socketBuffer))) //Client is already in info map
	{
		CLIENT_INFO info;
		clientInformationsMap->Get((const char*)(socketBuffer), &info);

		if (info.fileDataArraySize == info.ownedFilesCount)
		{
			info.clientOwnedFiles = (FILE_DATA*)realloc(info.clientOwnedFiles, info.fileDataArraySize + FILE_PARTS);
			info.fileDataArraySize *= 2;
		}
		info.clientOwnedFiles[info.ownedFilesCount] = data;
		info.ownedFilesCount++;
		clientInformationsMap->Insert((const char*)(socketBuffer), info);
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
	clientInformationsMap->Insert((const char*)(socketBuffer), info);
	LeaveCriticalSection(&ClientInfoAccess);
	return 0;

}


int RemoveClientInfo(SOCKET clientSocket, HashMap<CLIENT_INFO>* clientInformationsMap)
{
	if (!isInitClientInfoHandle)
		return -2;

	//Convert socket to string to use as key
	char socketBuffer[SOCKET_BUFFER_SIZE];
	sprintf_s(socketBuffer, "%d", clientSocket);

	EnterCriticalSection(&ClientInfoAccess);
	if (clientInformationsMap->DoesKeyExist((const char*)(socketBuffer))) //Client is already in info map
	{
		CLIENT_INFO info;
		clientInformationsMap->Get((const char*)(socketBuffer), &info);

		free(info.clientOwnedFiles);
		clientInformationsMap->Delete((const char*)(socketBuffer));
		LeaveCriticalSection(&ClientInfoAccess);
		return 0;

	}
	else {
		LeaveCriticalSection(&ClientInfoAccess);
		return -1;
	}
	LeaveCriticalSection(&ClientInfoAccess);
}

int ClearClientInfoMap(HashMap<CLIENT_INFO>* clientInformationsMap)
{
	int keysCount = 0;
	char** keys = clientInformationsMap->GetKeys(&keysCount);

	if (keysCount == 0)
		return 0;

	for (int i = 0; i < keysCount; i++)
	{
		CLIENT_INFO info;
		clientInformationsMap->Get(keys[i], &info);
		free(info.clientOwnedFiles);
		free(keys[i]);
	}

	free(keys);
	return 0;
}