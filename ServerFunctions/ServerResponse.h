#pragma once
#include "../DataStructures/HashMap.h"
#include "../ServerFunctions/ServerStructs.h"


int PackExistingFileResponse(FILE_RESPONSE* response, FILE_DATA fileData, FILE_REQUEST request, int* serverOwnedParts, HashMap<FILE_DATA>* fileInfoMap, CRITICAL_SECTION* fileDataAccess);