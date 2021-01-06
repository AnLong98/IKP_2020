#define _CRT_SECURE_NO_WARNINGS 1
#include "../FileIO_Functions/FileIO.h"
#include <stdlib.h>
#include <stdio.h>
#include <direct.h>

int ReadFileIntoMemory(char* fileName, char** unallocatedBufferPointer, size_t* size)
{
	char path[80];
	_getcwd(path, 80);
	printf("Current Directory = %s", path);

	FILE* pFile;
	pFile = fopen(fileName, "rb");

	if (pFile == NULL)
	{
		return -1;
	}

	fseek(pFile, 0L, SEEK_END);
	*size = ftell(pFile);
	fseek(pFile, 0L, SEEK_SET);

	*unallocatedBufferPointer = (char*)malloc((*size) * sizeof(char));
	size_t readBytes = fread(*unallocatedBufferPointer, 1 ,(*size) , pFile);

	fclose(pFile);
	if (readBytes != *size)
	{
		free(*unallocatedBufferPointer);
		return -1;
	}

	return 0;

}


int WriteFileIntoMemory(char* fileName, char* allocatedBufferPointer, size_t size)
{
	FILE* pFile;
	pFile = fopen(fileName, "wb");

	if (pFile == NULL)
	{
		return -1;
	}
	size_t bytesWritten = fwrite(allocatedBufferPointer, size, 1, pFile);

	fclose(pFile);
	if (bytesWritten != size)
	{
		return -1;
	}

	return 0;
}