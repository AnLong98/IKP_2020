#pragma once

/*
	Reads file into RAM memory. Allocates memory buffer needed to store whole file.
	fileName - Name of file to be read
	unallocatedBufferPointer - Pointer in which file will be stored, memory will be allocated inside function and assigned to this pointer
	size - Pointer to size object in which file size will be stored

	Returns 0 if reading is successful, otherwise returns -1
*/
int ReadFileIntoMemory(char* fileName, char** unallocatedBufferPointer, size_t* size);

/*
	Writes file onto disk. Free's allocated memory buffer.
	fileName - Name of file to be written
	unallocatedBufferPointer - Pointer in which file is stored, memory will be freed in function
	size - size of file in bytes

	Returns 0 if writing is successful, otherwise returns -1
*/
int WriteFileIntoMemory(char* fileName, char* allocatedBufferPointer, size_t size);
