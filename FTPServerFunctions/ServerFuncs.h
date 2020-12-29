#pragma once
#include <WinSock2.h>
int ShutdownConnection(SOCKET* socket);
int AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket);
int RemoveSocketFromArray(SOCKET acceptedSockets[], int *freeIndex);
