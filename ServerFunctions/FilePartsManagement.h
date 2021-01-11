#pragma once
#include "../ServerFunctions/ServerStructs.h"
#include "../DataStructures/HashMap.h"


int DivideFileIntoParts(char* loadedFileBuffer, size_t fileSize, unsigned int parts, FILE_PART** unallocatedPartsArray);
int AssignFilePartToClient(SOCKADDR_IN clientInfo, char* fileName, HashMap<FILE_DATA>* fileInfoMap, CRITICAL_SECTION* fileDataAccess);
int UnassignFilePart(SOCKADDR_IN clientInfo, char* fileName);