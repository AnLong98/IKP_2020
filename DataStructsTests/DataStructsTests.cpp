#pragma comment(lib, "Ws2_32.lib")
#include "../DataStructures/HashMap.h"
#include "../DataStructures/LinkedList.h"
#include "../DataStructures/Queue.h"
#include <conio.h>
#define QUEUE_TEST_THREADS 20
#define HASH_MAP_HARD_TEST 2
#define HASH_MAP_FULL_TEST 4
#define LIST_TEST_THREADS 50
#define MAP_TEST_THREADS 50
#define OPERATIONS_PER_THREAD 100

#include <iostream>
#include <string>
using namespace std;

DWORD WINAPI QueueTester(LPVOID params);
DWORD WINAPI HashMapTester(LPVOID params);
DWORD WINAPI ListTester(LPVOID params);

int TEST_RUNNING = 0; //Global variable to begin tests

int main()
{
	HANDLE queueTests[QUEUE_TEST_THREADS] = { NULL };
	HANDLE listTests[LIST_TEST_THREADS] = { NULL };
	HANDLE mapTests[MAP_TEST_THREADS] = { NULL };
	DWORD queueIDs[QUEUE_TEST_THREADS];
	DWORD listIDs[LIST_TEST_THREADS];
	DWORD mapIDs[MAP_TEST_THREADS];

	Queue<int> testQueue(50);
	HashMap<int> testMap(5);
	LinkedList<int> testList;

	//Create queue testers
	for (int i = 0; i < QUEUE_TEST_THREADS; i++)
	{
		queueTests[i] = CreateThread(NULL, 0, &QueueTester, (LPVOID)&testQueue, 0, queueIDs + i);
	}

	//Create list testers
	for (int i = 0; i < LIST_TEST_THREADS; i++)
	{
		listTests[i] = CreateThread(NULL, 0, &ListTester, (LPVOID)&testList, 0, listIDs + i);
	}

	//Create map testers
	for (int i = 0; i < MAP_TEST_THREADS; i++)
	{
		mapTests[i] = CreateThread(NULL, 0, &HashMapTester, (LPVOID)&testMap, 0, mapIDs + i);
	}

	TEST_RUNNING = 1;
	for (int i = 0; i < QUEUE_TEST_THREADS; i++)
	{
		WaitForSingleObject(queueTests[i], INFINITE);
	}

	for (int i = 0; i < LIST_TEST_THREADS; i++)
	{
		WaitForSingleObject(listTests[i], INFINITE);
	}

	for (int i = 0; i < MAP_TEST_THREADS; i++)
	{
		WaitForSingleObject(mapTests[i], INFINITE);
	}
	TEST_RUNNING = 0;

	printf("\nTest finished");
	_getch();
}

DWORD WINAPI QueueTester(LPVOID params)
{
	Queue<int>* testQueue = (Queue<int>*)params;
	while (!TEST_RUNNING)
		Sleep(100);
	for (int i = 0; i < OPERATIONS_PER_THREAD; i++)
	{
		int operation = rand() % 3;
		int value, op;
		switch (operation)
		{
		case 0:
			value = rand() % 100;
			op = testQueue->Enqueue(value);
			printf("\nEnqueue operation by thread %d was %d, value added %d", GetCurrentThreadId(), op, value);
			break;
		case 1:
			testQueue->Dequeue();
			printf("\nDequeue operation by thread %d", GetCurrentThreadId());
			break;
		case 2:
			op = testQueue->DequeueGet(&value);
			printf("\nDequeueGet operation by thread %d was %d, value received %d", GetCurrentThreadId(), op, value);
		}


	}
	printf("\nThread %d finished", GetCurrentThreadId());
	return 0;
}


DWORD WINAPI HashMapTester(LPVOID params)
{
	//Array of test keys
	const char * testKeys[] = {
	"Test100KB.txt","Test200KB.txt","Test300KB.txt",
	"Test400KB.txt","Test500KB.txt","Test600KB.txt",
	"Test700KB.txt","Test800KB.txt","Test900KB.txt",
	"Test1MB.txt","Test2MB.txt","Test3MB.txt",
	"Test4MB.txt","Test5MB.txt","Test6MB.txt",
	"Test7MB.txt","Test100MB.txt","Test200MB.txt",
	"Test300MB.txt","Test400MB.txt","Test500MB.txt",
	"Test600MB.txt","Test700MB.txt","Test800MB.txt",
	"Test900MB.txt",
	};

	HashMap<int>* testMap = (HashMap<int>*)params;
	while (!TEST_RUNNING)
		Sleep(100);
	for (int i = 0; i < OPERATIONS_PER_THREAD; i++)
	{
		int operation = rand() % HASH_MAP_HARD_TEST; //Put HASH_MAP_FULL_TEST for more thorough but less intensive test
		int key;
		int value;
		int op;
		switch (operation)
		{
		case 0:
			value = rand() % 100;
			key = rand() % 25;
			testMap->Insert(testKeys[key], value);
			printf("\nInsert operation by thread %d with key %s, value added %d", GetCurrentThreadId(), testKeys[key], value);
			break;
		case 1:
			key = rand() % 25;
			testMap->Delete(testKeys[key]);
			printf("\nDelete  operation by thread %d for key %s", GetCurrentThreadId(), testKeys[key]);
			break;
		case 2:
			value = rand() % 100;
			key = rand() % 25;
			op = testMap->Get(testKeys[key], &value);
			printf("\nGet operation by thread %d was %d for key %s, value received %d", GetCurrentThreadId(), op, testKeys[key], value);
			break;
		case 3:
			key = rand() % 25;
			op = testMap->DoesKeyExist(testKeys[key]);
			printf("\nDoes key exist operation by thread %d was %d for key %s", GetCurrentThreadId(), op, testKeys[key]);
		}
	}
	printf("\nThread finished");
	return 0;
}

DWORD WINAPI ListTester(LPVOID params)
{
	LinkedList<int>* testList = (LinkedList<int>*)params;
	while (!TEST_RUNNING)
		Sleep(100);
	for (int i = 0; i < OPERATIONS_PER_THREAD; i++)
	{
		int operation = rand() % 4;
		int value, op;
		switch (operation)
		{
		case 0:
			value = rand() % 100;
			op = testList->PushFront(value); 
			printf("\nPushFront operation is %d by thread %d, value added %d", op,  GetCurrentThreadId(), value);
			break;
		case 1:
			value = rand() % 100;
			op = testList->PushBack(value);
			printf("\nPusBack operation is %d by thread %d, value added %d", op, GetCurrentThreadId(), value);
			break;
		case 2:
			value;
			op = testList->PopFront(&value);
			printf("\nPopFront operation is %d by thread %d, value got %d", op, GetCurrentThreadId(), value);
			break;
		case 3:
			value;
			op = testList->PopBack(&value);
			printf("\nPopBack operation is %d by thread %d, value got %d", op, GetCurrentThreadId(), value);
			break;
		}
	}
	printf("\nThread finished");
	return 0;
}

