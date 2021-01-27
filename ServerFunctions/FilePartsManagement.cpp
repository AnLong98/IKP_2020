#include "../ServerFunctions/FilePartsManagement.h"
#include "../DataStructures/HashMap.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#define IP_BUFFER_SIZE 16

CRITICAL_SECTION FileInfoAccess;
int isInitFileManagementHandle = 0;

void InitFileManagementHandle()
{
	InitializeCriticalSection(&FileInfoAccess);
	isInitFileManagementHandle = 1;
}

void DeleteFileManagementHandle()
{
	DeleteCriticalSection(&FileInfoAccess);
	isInitFileManagementHandle = 0;
}


int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName, HashMap<FILE_DATA>* fileInfoMap)
{
	
	if (!isInitFileManagementHandle)
		return -2;

	int partAssigned = 0;
	FILE_DATA fileData;

	EnterCriticalSection(&FileInfoAccess);
	if (!fileInfoMap->Get(fileName, &fileData))
	{
		LeaveCriticalSection(&FileInfoAccess);
		return -1;
	}
	if (fileData.partArraySize - 1 == fileData.partsOnClients)//Parts array is full and should be increased
	{
		FILE_PART* tmp = (FILE_PART*)realloc(fileData.filePartDataArray, fileData.partArraySize + FILE_PARTS); //Increase array by the count of file parts
		if (tmp == NULL)
			return -1;
		fileData.filePartDataArray = tmp;
		fileData.partArraySize += FILE_PARTS;
	}
	if (fileData.filePartDataArray == NULL)
	{
		LeaveCriticalSection(&FileInfoAccess);
		return -1;
	}
	partAssigned = fileData.nextPartToAssign;

	int canBeAssigned = 0;
	int clientFilePartIndex = -1;

	//Find part to assign
	for (int i = 0; i < fileData.partArraySize; i++)
	{
		if (fileData.filePartDataArray[i].isServerOnly)
		{
			canBeAssigned = 1;
			clientFilePartIndex = i;
			partAssigned = clientFilePartIndex % FILE_PARTS;
			break;
		}
	}

	if (!canBeAssigned)
		return -1;
	
	fileData.nextPartToAssign = (fileData.nextPartToAssign + 1) % FILE_PARTS;									//Set next part to assign
	
	FILE_PART filePartToAssign = fileData.filePartDataArray[partAssigned];									//Get data from first 10 assigned file parts
	fileData.filePartDataArray[clientFilePartIndex].clientOwnerAddress = clientInfo;						//Assign new client's connection info
	fileData.filePartDataArray[clientFilePartIndex].isServerOnly = 0;										//Mark as client owned
	fileData.filePartDataArray[clientFilePartIndex].partSize = filePartToAssign.partSize;					//Copy part size
	fileData.filePartDataArray[clientFilePartIndex].partStartPointer = filePartToAssign.partStartPointer;	//Copy buffer pointer
	fileData.filePartDataArray[clientFilePartIndex].filePartNumber = filePartToAssign.filePartNumber;		//Copy part number	
	memcpy(fileData.fileName, fileName, strlen(fileName) + 1);													//Copy file name
	printf("\nFor file %s and new client, Client part index %d", fileData.fileName, clientFilePartIndex);
	fileData.partsOnClients++;
	fileInfoMap->Insert(fileName,fileData);																	//Save changes
	LeaveCriticalSection(&FileInfoAccess);
	return partAssigned;
}


int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART** unallocatedPartsArray)
{
	if (!isInitFileManagementHandle)
		return -2;

	size_t partSize = fileSize / parts;
	size_t totalSizeAccounted = 0;

	*unallocatedPartsArray = (FILE_PART*)malloc(sizeof(FILE_PART) * parts * 2); //Allocate double the size

	if (*unallocatedPartsArray == NULL)
	{
		return -1;
	}
	int partNumber = 0;
	for (int i = 0; i < (int)parts; i++)
	{
		if (i == parts - 1)
		{
			(*unallocatedPartsArray)[i].partSize = fileSize - totalSizeAccounted;
		}
		else
		{
			(*unallocatedPartsArray)[i].partSize = partSize;
		}
		(*unallocatedPartsArray)[i].filePartNumber = partNumber++;
		(*unallocatedPartsArray)[i].isServerOnly = 1; //Assign part to server first
		(*unallocatedPartsArray)[i].partStartPointer = loadedFileBuffer + i * partSize;
		totalSizeAccounted += partSize;
	}

	for (int i = parts; i < 2 * (int)parts; i++)
	{
		(*unallocatedPartsArray)[i].isServerOnly = 1;
	}

	return 0;
}


int UnassignFileParts(SOCKADDR_IN clientInfo, HashMap<FILE_DATA>* fileInfoMap, CLIENT_INFO info, unsigned int filePartsCount)
{
	if (!isInitFileManagementHandle)
		return -2;

	wchar_t filePartIpBuffer[IP_BUFFER_SIZE];
	wchar_t clientIpBuffer[IP_BUFFER_SIZE];
	int clientPartIndex = -1;

	EnterCriticalSection(&FileInfoAccess);
	for (int i = 0; i < (int)filePartsCount; i++)//Iterate through all client owned files file data and unassign client's part from part's array
	{
		FILE_DATA fileData;
		if (!fileInfoMap->Get(info.clientOwnedFiles[i], &fileData))//Client does not have any file parts
		{
			return -1;
		}

		//Find index of user assigned part
		for (int i = 0; i < (int)fileData.partArraySize; i++)
		{
			if (fileData.filePartDataArray[i].isServerOnly)continue; //Skip server owned parts
			unsigned short filePartPort = fileData.filePartDataArray[i].clientOwnerAddress.sin_port;
			InetNtop(AF_INET, &(fileData.filePartDataArray[i].clientOwnerAddress.sin_addr), (PWSTR)filePartIpBuffer, IP_BUFFER_SIZE);
			InetNtop(AF_INET, &(clientInfo.sin_addr), (PWSTR)clientIpBuffer, IP_BUFFER_SIZE);
			if (filePartPort == clientInfo.sin_port && wcscmp(filePartIpBuffer, clientIpBuffer) == 0)
			{
				clientPartIndex = i;
				break;
			}
		}

		int filePartToShift = fileData.filePartDataArray[clientPartIndex].filePartNumber;
		if (clientPartIndex == -1)
		{
			LeaveCriticalSection(&FileInfoAccess);
			return -1;
		}


		int isShifted = 0;
		//Check if any other user has this file part and put his data here
		for (int i = clientPartIndex + 1; i < (int)fileData.partsOnClients; i++)
		{
			if (fileData.filePartDataArray[i].isServerOnly)continue;
			if (fileData.filePartDataArray[i].filePartNumber == filePartToShift)
			{
				fileData.filePartDataArray[clientPartIndex].clientOwnerAddress = fileData.filePartDataArray[i].clientOwnerAddress;
				clientPartIndex = i;
				isShifted = 1;

			}
		}

		fileData.filePartDataArray[clientPartIndex].isServerOnly = 1;


		fileData.partsOnClients--; //There is one less client owned part now.
		fileInfoMap->Insert(info.clientOwnedFiles[i], fileData);
		LeaveCriticalSection(&FileInfoAccess);
	}
	
	return 0;
}


int PackExistingFileResponse(FILE_RESPONSE* response, const char* fileName, FILE_REQUEST request, int* serverOwnedParts, HashMap<FILE_DATA>* fileInfoMap, FILE_PART* partsToSendCopy)
{
	int serverOwnedPartsCount = 0;
	int clientOwnedPartsCount = 0;

	EnterCriticalSection(&FileInfoAccess);
	FILE_DATA fileData;
	fileInfoMap->Get(fileName, &fileData);
	for (int i = 0; i < FILE_PARTS; i++)
	{
		partsToSendCopy[i] = fileData.filePartDataArray[i];
		if (fileData.filePartDataArray[i].isServerOnly)
		{
			serverOwnedParts[serverOwnedPartsCount++] = i;
		}
		else
		{
			FILE_PART_INFO partInfo;
			partInfo.partNumber = fileData.filePartDataArray[i].filePartNumber;
			partInfo.clientOwnerAddress = fileData.filePartDataArray[i].clientOwnerAddress;
			response->clientParts[clientOwnedPartsCount++] = partInfo;
		}

	}
	LeaveCriticalSection(&FileInfoAccess);
	response->fileExists = 1;
	response->clientPartsNumber = clientOwnedPartsCount;
	response->serverPartsNumber = serverOwnedPartsCount;
	response->fileSize = fileData.fileSize;

	int assignedPart = AssignFilePartToClient(request.requesterListenAddress, request.fileName, fileInfoMap);
	if (assignedPart == -1)
	{
		return -1;
	}
	else
	{
		response->filePartToStore = assignedPart;
	}

	return 0;

}

int ClearFileInfoMap(HashMap<FILE_DATA>* fileInfoMap)
{
	int keysCount = 0;
	char** keys = fileInfoMap->GetKeys(&keysCount);

	if (keysCount == 0)
		return 0;

	for (int i = 0; i < keysCount; i++)
	{
		FILE_DATA fileData;
		fileInfoMap->Get(keys[i], &fileData);
		free(fileData.filePartDataArray);
		free(fileData.filePointer);
		fileInfoMap->Delete(keys[i]);
		free(keys[i]);
	}

	free(keys);
	return 0;
}