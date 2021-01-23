#pragma once

#ifndef CONNECTIONFUNCS_H
#define CONNECTIONFUNCS_H

#include <WinSock2.h>
#include "../DataStructures/Queue.h"
#include "../DataStructures/HashMap.h"
#include "../DataStructures/LinkedList.h"

/*
	Initializes library handle, and allows thread safe access. Needs to be called ONCE before use.
*/
void InitConnectionsHandle();

/*
	Deletes library handle, after calling this library will become unusable.
*/
void DeleteConnectionsHandle();

/*
	Shuts down connection for both sides on passed socket.
	socket - Socket to shut down 
	Returns 0 if no errors, otherwise returns -1
*/
int ShutdownConnection(SOCKET socket);


/*
	Accepts incoming connection request on listen socket and adds it to accepted sockets array.
	socketsList - Sockets array in which accepted socket will be placed
	listenSocket - socket from which connection request will be accepted

	Returns 0 if successful, otherwise returns -1
*/
// THREAD UNSAFE
int AcceptIncomingConnection(LinkedList<SOCKET>* socketsList, SOCKET listenSocket);

/*
	Removes socket from array.
	socketsList - Sockets array in which accepted socket is stored.
	socket - Socket to remove

	Returns 0 if successful, otherwise returns -1
*/
// THREAD UNSAFE
int RemoveSocketFromArray(LinkedList<SOCKET>* socketsList, SOCKET socket);

/*
	Returns all sockets in list

	socketsArray - SocketsList
	socketsCount - pointer in which to store number of sockets in list

	Returns NULL if empty
*/
SOCKET* GetAllSockets(LinkedList<SOCKET>* socketsArray, int* socketsCount);


/*
	Checks which sockets from acceptedSockets list are set and places them onto queue.
	socketsList - Sockets list.
	communicationQueue - Queue on which to place set sockets.
	processingSocketsMap - Map in which to place set sockets.

	Returns number of set sockets or -1 in case of failiure
*/
// THREAD UNSAFE
int CheckSetSockets(LinkedList<SOCKET>* socketsList, fd_set* readfds, Queue<SOCKET>* communicationQueue, HashMap<SOCKET>* processingSocketsMap);

/*
	Disconnects socket's that are broken (if any), shuts them down and removes them from array of sockets. Should be called if select() returns SOCKET_ERROR

	socketsList - Sockets array
	socketsCount - number of sockets in array

	Returns number of disconnected sockets

	THREAD UNSAFE
*/
int DisconnectBrokenSockets(LinkedList<SOCKET>* socketsList);

/*
	Checks if socket and underlaying connection is broken.

	s - Socket to be checked

	Returns 1 if broken, 0 if not broken

	THREAD UNSAFE
*/
int IsSocketBroken(SOCKET s);


#endif