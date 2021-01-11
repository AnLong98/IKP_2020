#include "../ServerFunctions/ClientInfoManagement.h"

int AddClientInfo(SOCKET* socket, FILE_DATA data, SOCKADDR_IN clientAddress, LinkedList<CLIENT_INFO>* clientInformationsList )
{
	//TODO ADD user's socket info 

	int result;
	ListNode<CLIENT_INFO> iterator = clientInformationsList->AcquireIteratorNodeFront(&result);
	if (result != 0)//List is empty
	{

		for (int i = 0; i < clientInformationsList->Count(); i++) {
			CLIENT_INFO info = iterator.GetValue();
			if (info.clientSocket == socket) //Client's socket already exists here so we will just append new data
			{
				if (info.fileDataArraySize == info.ownedFilesCount)
				{
					info.clientOwnedFiles = (FILE_DATA*)realloc(info.clientOwnedFiles, info.fileDataArraySize + FILE_PARTS);
					info.fileDataArraySize *= 2;
				}
				info.clientOwnedFiles[info.ownedFilesCount] = data;
				info.ownedFilesCount++;
				clientInformationsList->ReleaseIterator();
				clientInformationsList->RemoveElement(iterator);
				clientInformationsList->PushFront(info);
				return 0;
			}
			iterator = iterator.Next();
		}
	}
	clientInformationsList->ReleaseIterator();

	CLIENT_INFO info;
	info.clientOwnedFiles = (FILE_DATA*)malloc(FILE_PARTS * sizeof(FILE_DATA));
	info.clientAddress = clientAddress;
	info.clientSocket = socket;
	info.fileDataArraySize = FILE_PARTS;
	info.ownedFilesCount = 1;
	info.clientOwnedFiles[0] = data;
	clientInformationsList->PushFront(info);
	return 0;

}


int RemoveClientInfo(SOCKET* clientSocket, LinkedList<CLIENT_INFO>* clientInformationsList)
{

	int result;
	ListNode<CLIENT_INFO> iterator = clientInformationsList->AcquireIteratorNodeFront(&result);

	if (result == 0)
	{
		clientInformationsList->ReleaseIterator();
		return -1;
	}

	for (int j = 0; j < clientInformationsList->Count(); j++) {
		CLIENT_INFO info = iterator.GetValue();
		if (info.clientSocket == clientSocket)
		{
			for (int i = 0; i < info.ownedFilesCount; i++)
			{

				UnassignFilePart(info.clientAddress, info.clientOwnedFiles[i].fileName);

			}
			free(info.clientOwnedFiles);
			clientInformationsList->ReleaseIterator();
			clientInformationsList->RemoveElement(iterator);
			return 0;
		}
	}


	return -1;
}