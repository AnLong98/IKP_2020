#pragma once
#include <WinSock2.h>
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
