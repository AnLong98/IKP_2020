#pragma once
#include "../DataStructures/LinkedList.h"
#include "../ServerFunctions/ServerStructs.h"
#include "../ServerFunctions/FilePartsManagement.h"
#include <winsock2.h>

int AddClientInfo(SOCKET* socket, FILE_DATA data, SOCKADDR_IN clientAddress, LinkedList<CLIENT_INFO>* clientInformationsList);
int RemoveClientInfo(SOCKET* clientSocket, LinkedList<CLIENT_INFO>* clientInformationsList);