#pragma once
#include <WinSock2.h>
int ShutdownConnection(SOCKET* socket);
int AcceptIncomingConnection(SOCKET acceptedSockets[], int *freeIndex, SOCKET listenSocket, fd_set* readfds);
