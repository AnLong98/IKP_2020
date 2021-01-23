#pragma once

#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <iostream>
#include <cstdlib>
#include <Windows.h>
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
	ListNode();
	T GetValue();
	ListNode* Next();
	ListNode* Previous();

	template <class T> friend class LinkedList;
};

// Class for Linked list
template <class T>
class LinkedList
{
private:
	CRITICAL_SECTION ListCS;			 //List mutex
	ListNode<T>* head;			// List head
	ListNode<T>* rear;			// List rear
	int count;					// current size of the list

public:
	LinkedList();											//CTOR
	~LinkedList();											//DTOR
	bool PushFront(T element);								//Add element to the front of the list, returns true if successfull
	bool PushBack(T element);								//Add element to the rear of the list, returns true if successfull
	ListNode<T>* AcquireIteratorNodeBack();					//returns back node pointer
	ListNode<T>* AcquireIteratorNodeFront();				//returns front node pointer
	bool PopFront(T* value);								//Gets element from the front of the list, returns true if list is not empty
	bool PopBack(T* value);									//Gets element from the rear of the list, returns true if list is not empty
	bool RemoveElement(ListNode<T>* node);					//Removes element stored in the node, returns false if not found
	int Count();											//Get list elements count
	bool isEmpty();											//Check if empty
};

// Constructor to initialize list node
template <class T>
ListNode<T>::ListNode()
{
	this->next = nullptr;
	this->previous = nullptr;
}


// Function to get value
template <class T>
T ListNode<T>::GetValue()
{
	return value;
}


template <class T>
ListNode<T>* ListNode<T>::Next()
{
	return next;
}

template <class T>
ListNode<T>* ListNode<T>::Previous()
{
	return previous;
}

// Constructor to initialize list
template <class T>
LinkedList<T>::LinkedList()
{
	head = nullptr;
	count = 0;
	rear = head;
	InitializeCriticalSection(&ListCS);

}

// Constructor to initialize list
template <class T>
LinkedList<T>::~LinkedList()
{
	ListNode<T>* pointer = head;
	if (pointer != nullptr)
	{
		ListNode<T>* pointerNext = head->next;
		while (pointerNext != nullptr)
		{
			delete pointer;
			pointer = pointerNext;
			pointerNext = pointerNext->next;
		}
		delete pointer;


	}

	DeleteCriticalSection(&ListCS);
		

}



// Utility function to check if the list is empty
template <class T>
bool LinkedList<T>::isEmpty()
{
	return Count() == 0;
}


// Utility function to return list elements count
template <class T>
int LinkedList<T>::Count()
{
	return count;
}

// Utility function to push new elemnt to the front of the list
template <class T>
bool LinkedList<T>::PushFront(T element)
{
	EnterCriticalSection(&ListCS);

	if (count == 0)
	{
		ListNode<T>* newNode = new ListNode<T>();
		newNode->value = element;
		newNode->previous = nullptr;
		newNode->next = nullptr;
		head = newNode;
		rear = newNode;
		count++;
		LeaveCriticalSection(&ListCS);
		return true;
	}
	ListNode<T>* newNode = new ListNode<T>();
	newNode->value = element;
	newNode->previous = nullptr;
	newNode->next = head;
	head->previous = newNode;
	head = newNode;
	count++;
	LeaveCriticalSection(&ListCS);
	return true;
}

// Utility function to push new elemnt to the back of the list
template <class T>
bool LinkedList<T>::PushBack(T element)
{
	EnterCriticalSection(&ListCS);

	if (count == 0)
	{
		ListNode<T>* newNode = new ListNode<T>();
		newNode->value = element;
		newNode->previous = nullptr;
		newNode->next = nullptr;
		head = newNode;
		rear = newNode;
		count++;
		LeaveCriticalSection(&ListCS);
		return true;
	}
	ListNode<T>* newNode = new ListNode<T>();
	newNode->value = element;
	newNode->previous = rear;
	newNode->next = nullptr;
	rear->next = newNode;
	rear = newNode;
	count++;
	LeaveCriticalSection(&ListCS);
	return true;
}

// Utility function to pop element from the front of the list
template <class T>
bool LinkedList<T>::PopFront(T* value)
{
	EnterCriticalSection(&ListCS);
	if (count == 0)
	{
		LeaveCriticalSection(&ListCS);
		return false;
	}
	ListNode<T>* frontNode = head;
	*value = frontNode->value;
	if (count == 1)
	{
		head = nullptr;
		rear = nullptr;
	}
	else
	{
		head = frontNode->next;
		head->previous = nullptr;
	}
	frontNode->next = nullptr;
	frontNode->previous = nullptr;
	delete frontNode;
	count--;
	LeaveCriticalSection(&ListCS);
	return true;
}

// Utility function to pop element from the back of the list
template <class T>
bool LinkedList<T>::PopBack(T* value)
{
	EnterCriticalSection(&ListCS);
	if (count == 0)
	{
		LeaveCriticalSection(&ListCS);
		return false;
	}
	ListNode<T>* backNode = rear;
	*value = backNode->value;
	if (count == 1)
	{
		head = nullptr;
		rear = nullptr;
	}
	else
	{
		rear = backNode->previous;
		rear->next = nullptr;
	}
	backNode->next = nullptr;
	backNode->previous = nullptr;
	delete backNode;
	count--;
	LeaveCriticalSection(&ListCS);
	return true;
}

// Utility function to remove list element
template <class T>
bool LinkedList<T>::RemoveElement(ListNode<T>* node)
{
	EnterCriticalSection(&ListCS);
	if (count == 0)
	{
		LeaveCriticalSection(&ListCS);
		return false;
	}
	ListNode<T>* search = head;

	for (int i = 0; i < count; i++)
	{
		if (search == node && i == 0) //Remove head
		{
			if (count == 1)//There is only one element in list
			{
				head = nullptr;
				rear = nullptr;
				delete search;
				count--;
				break;
			}
			else
			{
				head = search->next;
				(search->next)->previous = nullptr;
				search->next = nullptr;
				search->previous = nullptr;
				delete search;
				count--;
				break;
			}
		}else if(search == node && i == count - 1)//Delete rear element
		{
			if (count == 1)//There is only one element in list
			{
				head = nullptr;
				rear = nullptr;
				search->next = nullptr;
				search->previous = nullptr;
				delete search;
				count--;
				break;
			}
			else
			{
				rear = search->previous;
				(search->previous)->next = nullptr;
				search->next = nullptr;
				search->previous = nullptr;
				delete search;
				count--;
				break;
			}
		}
		else if (search == node) //Delete arbitrary node
		{
			(search->previous)->next = search->next;
			(search->next)->previous = search->previous;
			search->next = nullptr;
			search->previous = nullptr;
			delete search;
			count--;
			break;
		}

		search = search->next;
	}
	LeaveCriticalSection(&ListCS);
	return false;
}

// Utility function to acquire iterator to rear, 0 is placed in result if acquisition failed
template <class T>
ListNode<T>* LinkedList<T>::AcquireIteratorNodeBack()
{
	EnterCriticalSection(&ListCS);
	if (rear != nullptr)
	{

		LeaveCriticalSection(&ListCS);
		return rear;
	}

	LeaveCriticalSection(&ListCS);
	return nullptr;
}

// Utility function to acquire iterator to front, 0 is placed in result if acquisition failed
template <class T>
ListNode<T>* LinkedList<T>::AcquireIteratorNodeFront()
{
	EnterCriticalSection(&ListCS);
	if (head != nullptr)
	{
		LeaveCriticalSection(&ListCS);
		return head;
	}

	LeaveCriticalSection(&ListCS);
	return nullptr;
}

#endif
