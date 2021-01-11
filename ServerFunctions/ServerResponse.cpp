#include "../ServerFunctions/ServerResponse.h"
#include "../ServerFunctions/FilePartsManagement.h"

int PackExistingFileResponse(FILE_RESPONSE* response, FILE_DATA fileData, FILE_REQUEST request, int* serverOwnedParts, HashMap<FILE_DATA>* fileInfoMap, CRITICAL_SECTION* fileDataAccess)
{
	int serverOwnedPartsCount = 0;
	int clientOwnedPartsCount = 0;

	for (int i = 0; i < FILE_PARTS; i++)
	{
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
	response->fileExists = 1;
	response->clientPartsNumber = clientOwnedPartsCount;
	response->serverPartsNumber = serverOwnedPartsCount;
	response->fileSize = fileData.fileSize;

	int assignedPart = AssignFilePartToClient(request.requesterListenAddress, request.fileName, fileInfoMap, fileDataAccess);
	if (assignedPart == -1)
	{
		//HANDLE error
		return -1;
	}
	else
	{
		response->filePartToStore = assignedPart;
	}

	return 0;

}