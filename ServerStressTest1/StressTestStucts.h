#pragma once

typedef struct TEST_REQUEST_SENDER_DATA
{
	unsigned int firstPortNumber;
	unsigned int lastPortNumber;
	const char** testFileNames;
	unsigned int testFileCount;
	int* threadsWorking;
	int threadID;
}T_REQ_DATA;
