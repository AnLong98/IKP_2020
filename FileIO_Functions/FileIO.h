#pragma once
int ReadFileIntoMemory(char* fileName, char* unallocatedBufferPointer, size_t* size);
int WriteFileIntoMemory(char* fileName, char* allocatedBufferPointer, size_t size);
