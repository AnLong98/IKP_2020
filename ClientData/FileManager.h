#pragma once
#pragma warning(disable:4996)
#include "../ClientData/Client_Structs.h"
#include "../DataStructures/LinkedList.h"
#include <Windows.h>
//#define _CRT_SECURE_NO_WARNINGS 1

using namespace std;
/*

*/
void InitFileAcessManagementHandle();

/*

*/
void DeleteFileAccessManagementHandle();

/*

*/
void InitWholeFileManagementHandle();

/*

*/
void DeleteWholeFileManagementHandle();


/*

*/
int HandleRecievedFilePart(CLIENT_DOWNLOADING_FILE* wholeFile, char* data, int length, int partNumber, LinkedList<CLIENT_FILE_PART_INFO>* fileParts);

/*

*/
int InitWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile, char* fileName, FILE_RESPONSE response);

/*

*/
void ResetWholeFile(CLIENT_DOWNLOADING_FILE* wholeFile);

/*

*/
int FindFilePart(LinkedList<CLIENT_FILE_PART_INFO>* fileParts, CLIENT_FILE_PART_INFO* partToSend, char fileName[]);

/*

*/
int WriteWholeFileIntoMemory(char* dirName, CLIENT_DOWNLOADING_FILE wholeFile);

