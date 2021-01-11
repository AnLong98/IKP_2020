#include "../ServerFunctions/FilePartsManagement.h"
#include "../DataStructures/HashMap.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1


int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName, HashMap<FILE_DATA>* fileInfoMap, CRITICAL_SECTION* fileDataAccess )
{
	int partAssigned = 0;
	FILE_DATA fileData;

	fileInfoMap->Get(fileName, &fileData);
	EnterCriticalSection(fileDataAccess);
	if (fileData.partArraySize == fileData.partsOnClients)//Parts array is full and should be increased
	{
		fileData.filePartDataArray = (FILE_PART*)realloc(fileData.filePartDataArray, fileData.partArraySize + FILE_PARTS); //Increase array by the count of file parts
		fileData.partArraySize += FILE_PARTS;
	}
	if (fileData.filePartDataArray == NULL)
	{
		//ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
		LeaveCriticalSection(fileDataAccess);
		printf("\nReleased finish");
		return -1;
	}
	partAssigned = fileData.nextPartToAssign;
	fileData.partsOnClients++;																					//Increase number of parts owned by clients
	fileData.nextPartToAssign = (fileData.nextPartToAssign + 1) % FILE_PARTS;
	LeaveCriticalSection(fileDataAccess);
	
	FILE_PART filePartToAssign = fileData.filePartDataArray[fileData.nextPartToAssign];							//Get data from first 10 assigned file parts
	fileData.filePartDataArray[fileData.partsOnClients].clientOwnerAddress = clientInfo;						//Assign new client's connection info
	fileData.filePartDataArray[fileData.partsOnClients].isServerOnly = 0;										//Mark as client owned
	fileData.filePartDataArray[fileData.partsOnClients].partSize = filePartToAssign.partSize;					//Copy part size
	fileData.filePartDataArray[fileData.partsOnClients].partStartPointer = filePartToAssign.partStartPointer;	//Copy buffer pointer
	fileData.filePartDataArray[fileData.partsOnClients].filePartNumber = filePartToAssign.filePartNumber;		//Copy part number
	//fileData.partsOnClients++;																					//Increase number of parts owned by clients
	//fileData.nextPartToAssign = (fileData.nextPartToAssign + 1) % FILE_PARTS;									//Set next part to assign
	memcpy(fileData.fileName, fileName, strlen(fileName));														//Copy file name
	
	fileInfoMap->Insert(fileName,fileData);																			//Save changes

	return partAssigned;
}


int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART** unallocatedPartsArray)
{
	size_t partSize = fileSize / parts;
	size_t totalSizeAccounted = 0;

	*unallocatedPartsArray = (FILE_PART*)malloc(sizeof(FILE_PART) * parts * 2); //Allocate double the size

	if (*unallocatedPartsArray == NULL)
	{
		//ReleaseSemaphore(FinishSignal, SERVER_THREADS, NULL);
		printf("\nReleased finish");
		return -1;
	}
	int partNumber = 0;
	for (int i = 0; i < parts; i++)
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

	for (int i = parts; i < 2 * parts; i++)
	{
		(*unallocatedPartsArray)[i].isServerOnly = 1;
	}

	return 0;
}


int UnassignFilePart(SOCKADDR_IN clientInfo, char* fileName, HashMap<FILE_DATA>* fileInfoMap, CRITICAL_SECTION* fileDataAccess)
{
	PWSTR ipBuffer[16];
	int clientPartIndex = -1;
	FILE_DATA fileData;
	fileInfoMap->Get(fileName, &fileData);
	//Find index of user assigned part
	for (int i = 0; i < fileData.partsOnClients; i++)
	{
		if (fileData.filePartDataArray[i].isServerOnly)continue; //Skip server owned parts
		unsigned short filePartPort = fileData.filePartDataArray[i].clientOwnerAddress.sin_port;
		InetNtop(AF_INET, &(fileData.filePartDataArray[i].clientOwnerAddress.sin_addr), *ipBuffer, 16);
		char *filePartIp = inet_ntoa(fileData.filePartDataArray[i].clientOwnerAddress.sin_addr);
		if (filePartPort == clientInfo.sin_port && strcmp(filePartIp, inet_ntoa(clientInfo.sin_addr)) == 0)
		{
			clientPartIndex = i;
			break;
		}
	}
	EnterCriticalSection(fileDataAccess);
	int filePartToShift = fileData.filePartDataArray[clientPartIndex].filePartNumber;
	if (clientPartIndex == -1)
	{
		LeaveCriticalSection(fileDataAccess);
		return -1;
	}


	int isShifted = 0;
	//Check if any other user has this file part and put his data here
	for (int i = clientPartIndex + 1; i < fileData.partsOnClients; i++)
	{
		if (fileData.filePartDataArray[i].isServerOnly)continue;
		if (fileData.filePartDataArray[i].filePartNumber == filePartToShift)
		{
			fileData.filePartDataArray[clientPartIndex].clientOwnerAddress = fileData.filePartDataArray[i].clientOwnerAddress;
			clientPartIndex = i;
			isShifted = 1;

		}
	}

	if (!isShifted)
	{
		fileData.filePartDataArray[filePartToShift].isServerOnly = 1;
	}

	//Shift back only if client's part is not among first 10 parts
	if (clientPartIndex >= FILE_PARTS)
	{
		//Shift remaining file parts back one place to fill the gap
		for (int i = clientPartIndex + 1; i < fileData.partsOnClients; i++)
		{
			fileData.filePartDataArray[i - 1] = fileData.filePartDataArray[i];
		}
	}

	fileData.partsOnClients--; //There is one less client owned part now.
	LeaveCriticalSection(fileDataAccess);

	fileInfoMap->Insert(fileName, fileData);
	
	return 0;
}