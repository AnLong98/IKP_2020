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
	ListNode* head;      // List head
	ListNode* rear;      // List rear
	int capacity;		// Maximum capacity of the list
	int count;			// current size of the liest
	ListNode* searchNode //Current node in search

public:
	LinkedList(int size = LIST_INITIAL_SIZE);        //CTOR
	bool PushFront(T element);						//Add element to the front of the list, returns true if successfull
	bool PushBack(T element);						//Add element to the rear of the list, returns true if successfull
	bool LockListForSearch();						//Locks list for search, unlock needs to be called after search has ended
	bool UnlockListForSearch();						//Unlocks list for search
	bool PopFront(T* value);						//Gets element from the front of the list, returns true if list is not empty
	bool PopBack(T* value);							//Gets element from the rear of the list, returns true if list is not empty
	int Size();										//Get queue size
	int Capacity();									//Get queue capacity
	bool isEmpty();									//Check if empty
};