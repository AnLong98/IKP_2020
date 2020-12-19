#pragma once
#define MAX_FILE_NAME 70

typedef struct FILE_PART_REQUEST
{
	char fileName[MAX_FILE_NAME];
	unsigned int partNumber;
};

typedef struct FILE_REQUEST
{
	char fileName[MAX_FILE_NAME];
};

typedef struct FILE_RESPONSE
{
	//Add information about client owned part here
	unsigned int fileExists;
	unsigned int clientPartsNumber;
	unsigned int serverPartsNumber;
	unsigned int filePartToStore;
};
