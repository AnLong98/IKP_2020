#pragma once
#include <iostream>
#include <cstdlib>
#include <mutex>
#define LIST_INITIAL_SIZE 20
using namespace std;

//class for linked list node
template <class T>
class ListNode
{
	T value;
	ListNode* next;
	ListNode* previous;
};

// Class for Linked list
template <class T>
class LinkedList
{
private:
	mutex listMutex;	 //List mutex
	ListNode* head;      // array to store queue elements
	int capacity;		// Maximum capacity of the list
	int count;			// current size of the queue
	ListNode iterator;  //Iterator used to search list, can be used by only one thread at a given time

public:
	LinkedList(int size = LIST_INITIAL_SIZE);        //CTOR
	bool PushFront(T element);						//Add element to the front of the list, returns true if successfull
	bool PushBack(T element);						//Add element to the rear of the list, returns true if successfull
	bool GetIteratorNext(T* element );				//Locks list and places next element into "element" variable, returns false if end of list is reached
	bool ReleaseIterator();							//Unlocks list and resets iterator to the back of the list
	T PopFront();									//Gets element from the front of the list
	T PopBack();									//Gets element from the rear of the list
	int Size();										//Get queue size
	int Capacity();									//Get queue capacity
	bool isEmpty();									//Check if empty
};