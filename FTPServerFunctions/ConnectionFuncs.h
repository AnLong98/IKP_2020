#pragma once

#ifndef CONNECTIONFUNCS_H
#define CONNECTIONFUNCS_H

#include <WinSock2.h>
#include "../DataStructures/Queue.h"
#include "../DataStructures/HashMap.h"

/*
	Shuts down connection for both sides on passed socket.
	socket - Socket to shut down 
	Returns 0 if no errors, otherwise returns -1
*/
int ShutdownConnection(SOCKET* socket);


/*
	Accepts incoming connection request on listen socket and adds it to accepted sockets array.
	acceptedSockets - Sockets array in which accepted socket will be placed
	freeIndex - pointer to index of free place in array
	listenSocket - socket from which connection request will be accepted

	Returns 0 if successful, otherwise returns -1
*/
// THREAD UNSAFE
int AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket);

/*
	Removes socket from array.
	acceptedSockets - Sockets array in which accepted socket is stored.
	freeIndex - pointer to index of free place in array, will be decremented
	socket - Socket to remove

	Returns 0 if successful, otherwise returns -1
*/
// THREAD UNSAFE
int RemoveSocketFromArray(SOCKET acceptedSockets[], SOCKET* socket, int *freeIndex);


/*
	Checks which sockets from acceptedSockets list are set and places them onto queue.
	acceptedSockets - Sockets array in which accepted socket is stored.
	freeIndex - pointer to index of free place in array, will be decremented
	socket - Socket to remove

	Returns number of set sockets or -1 in case of failiure
*/
// THREAD UNSAFE
int CheckSetSockets(int* socketsTaken, SOCKET acceptedSockets[], fd_set* readfds, Queue<SOCKET*>* communicationQueue, HashMap<SOCKET*>* processingSocketsMap);

/*
	Disconnects socket's that are broken (if any), shuts them down and removes them from array of sockets. Should be called if select() returns SOCKET_ERROR

	sockets - Sockets array
	socketsCount - number of sockets in array

	Returns number of disconnected sockets

	THREAD UNSAFE
*/
int DisconnectBrokenSockets(SOCKET sockets[], int* socketsCount);

/*
	Checks if socket and underlaying connection is broken.

	s - Socket to be checked

	Returns 1 if broken, 0 if not broken

	THREAD UNSAFE
*/
int IsSocketBroken(SOCKET s);


#endif