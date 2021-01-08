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
private:
	T value;
	ListNode* next;
	ListNode* previous;
	
public:
	T GetValue();
	ListNode Next();
	ListNode Previous();

	template <class T> friend class LinkedList;
};

// Class for Linked list
template <class T>
class LinkedList
{
private:
	mutex listMutex;			 //List mutex
	ListNode<T>* head;			// List head
	ListNode<T>* rear;			// List rear
	int count;					// current size of the list

public:
	LinkedList(int size = LIST_INITIAL_SIZE);        //CTOR
	~LinkedList();									//DTOR
	bool PushFront(T element);						//Add element to the front of the list, returns true if successfull
	bool PushBack(T element);						//Add element to the rear of the list, returns true if successfull
	ListNode<T> AcquireIteratorNodeBack();			//returns back node and locks list, unlock needs to be called after search has ended
	ListNode<T> AcquireIteratorNodeFront();			//returns front node and locks list, unlock needs to be called after search has ended
	bool ReleaseIterator();							//Unlocks list for search
	bool PopFront(T* value);						//Gets element from the front of the list, returns true if list is not empty
	bool PopBack(T* value);							//Gets element from the rear of the list, returns true if list is not empty
	bool RemoveElement(ListNode<T> node);			//Removes element stored in the node, returns false if not found
	int Count();									//Get list elements count
	bool isEmpty();									//Check if empty
};