#pragma once
#include <ws2tcpip.h>
#include "../DataStructures/HashMap.h"
#include "../ServerFunctions/ServerStructs.h"
#include "../ServerFunctions/FilePartsManagement.h"
#include <Windows.h>


/*
	Initializes library handle, and allows thread safe access. Needs to be called ONCE before use. 
*/
void InitClientInfoHandle();

/*
	Deletes library handle, after calling this library will become unusable.
*/
void DeleteClientInfoHandle();

/*
	Adds informations about client and new file part assigned to him into hash map.
	socket - Client socket pointer
	data - Data about file client requested
	clientAddress - Client listen socket address
	clientInformationsMap - Hash map containing client informations, mapping socket address as key

	Returns 0 if successfull, -2 if library handle not initialized.
	THREAD SAFE
*/
int AddClientInfo(SOCKET* socket, FILE_DATA data, SOCKADDR_IN clientAddress, HashMap<CLIENT_INFO>* clientInformationsMap);

/*
	Removes informations about client and his file parts from hash map.
	clientSocket - Client socket pointer
	clientInformationsMap - Hash map containing client informations, mapping socket address as key

	Returns 0 if successfull,-2 if library handle not initialized, -1 if client not found.
	THREAD SAFE
*/
int RemoveClientInfo(SOCKET* clientSocket, HashMap<CLIENT_INFO>* clientInformationsMap);

/*
	Removes all client info
	clientInformationsMap - Hash map of client informations\

	Returns 0

	THREAD UNSAFE

*/
int ClearClientInfoMap(HashMap<CLIENT_INFO>* clientInformationsMap);