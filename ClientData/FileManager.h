#pragma once
#include "../ClientData/Client_Structs.h"


/*


*/

int HandleRecievedFilePart(CLIENT_FILE_PART_INFO* filePartInfo, CLIENT_DOWNLOADING_FILE* wholeFile, char* data, int length, int partNumber);



/*

*/
void WriteWholeFileIntoMemory(char* dirName, CLIENT_DOWNLOADING_FILE wholeFile);