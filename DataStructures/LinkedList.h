#pragma once
#include <iostream>
#include <cstdlib>
#include <mutex>
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
	LinkedList();											//CTOR
	~LinkedList();											//DTOR
	bool PushFront(T element);								//Add element to the front of the list, returns true if successfull
	bool PushBack(T element);								//Add element to the rear of the list, returns true if successfull
	ListNode<T> AcquireIteratorNodeBack(int* result);		//returns back node and locks list, unlock needs to be called after search has ended,
	ListNode<T> AcquireIteratorNodeFront(int* result);		//returns front node and locks list, unlock needs to be called after search has ended
	bool ReleaseIterator();									//Unlocks list for search
	bool PopFront(T* value);								//Gets element from the front of the list, returns true if list is not empty
	bool PopBack(T* value);									//Gets element from the rear of the list, returns true if list is not empty
	bool RemoveElement(ListNode<T> node);					//Removes element stored in the node, returns false if not found
	int Count();											//Get list elements count
	bool isEmpty();											//Check if empty
};

// Constructor to initialize list
template <class T>
ListNode<T>::ListNode()
{
	this->next = nullptr;
	this->previous = nullptr;
}


// Constructor to initialize list
template <class T>
T ListNode<T>::GetValue()
{
	return value;
}

// Constructor to initialize list
template <class T>
ListNode<T> ListNode<T>::Next()
{
	return *next;
}

// Constructor to initialize list
template <class T>
ListNode<T> ListNode<T>::Previous()
{
	return *previous;
}

// Constructor to initialize list
template <class T>
LinkedList<T>::LinkedList()
{
	head = nullptr;
	count = 0;
	rear = head;

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
	listMutex.lock();

	if (count == 0)
	{
		ListNode<T>* newNode = new ListNode<T>();
		newNode->value = element;
		newNode->previous = nullptr;
		newNode->next = nullptr;
		head = newNode;
		rear = newNode;
		count++;
		listMutex.unlock();
		return true;
	}
	ListNode<T>* newNode = new ListNode<T>();
	newNode->value = element;
	newNode->previous = nullptr;
	newNode->next = head;
	head->previous = newNode;
	head = newNode;
	count++;
	listMutex.unlock();
	return true;
}

// Utility function to push new elemnt to the back of the list
template <class T>
bool LinkedList<T>::PushBack(T element)
{
	listMutex.lock();

	if (count == 0)
	{
		ListNode<T>* newNode = new ListNode<T>();
		newNode->value = element;
		newNode->previous = nullptr;
		newNode->next = nullptr;
		head = newNode;
		rear = newNode;
		count++;
		listMutex.unlock();
		return true;
	}
	ListNode<T>* newNode = new ListNode<T>();
	newNode->value = element;
	newNode->previous = rear;
	newNode->next = nullptr;
	rear->next = newNode;
	rear = newNode;
	count++;
	listMutex.unlock();
	return true;
}

// Utility function to pop element from the front of the list
template <class T>
bool LinkedList<T>::PopFront(T* value)
{
	listMutex.lock();
	if (count == 0)
	{
		listMutex.unlock();
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
	listMutex.unlock();
	return true;
}

// Utility function to pop element from the back of the list
template <class T>
bool LinkedList<T>::PopBack(T* value)
{
	listMutex.lock();
	if (count == 0)
	{
		listMutex.unlock();
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
	listMutex.unlock();
	return true;
}

// Utility function to remove list element
template <class T>
bool LinkedList<T>::RemoveElement(ListNode<T> node)
{
	listMutex.lock();
	if (count == 0)
	{
		listMutex.unlock();
		return false;
	}
	ListNode<T>* search = head;

	for (int i = 0; i < count; i++)
	{
		if (search->next == node.next && search->previous == node.previous && (i == 0 || i == count - 1))
		{
			head = nullptr;
			rear = nullptr;
			count--;
			search->next == nullptr;
			search->previous = nullptr;
			delete search;
			listMutex.unlock();
			return true;
		}
		else if (search->next == node.next && search->previous == node.previous)
		{
			(search->previous)->next = search->next;
			(search->next)->previous = search->previous;
			count--;
			search->next = nullptr;
			search->previous = nullptr;
			delete search;
			listMutex.unlock();
			return true;
		}

		search = search->next;
	}
	listMutex.unlock();
	return false;
}

// Utility function to acquire iterator to rear, 0 is placed in result if acquisition failed
template <class T>
ListNode<T> LinkedList<T>::AcquireIteratorNodeBack(int* result)
{
	listMutex.lock();
	ListNode<T> nodeRet;
	if (rear != nullptr)
	{
		*result = 1;
		return *rear;
	}

	*result = 0;
	return nodeRet;
}

// Utility function to acquire iterator to front, 0 is placed in result if acquisition failed
template <class T>
ListNode<T> LinkedList<T>::AcquireIteratorNodeFront(int* result)
{
	listMutex.lock();
	ListNode<T> nodeRet;
	if (head != nullptr)
	{
		*result = 1;
		return *head;
	}

	*result = 0;
	return nodeRet;
}

// Utility function to release iterator
template <class T>
bool LinkedList<T>::ReleaseIterator()
{
	bool isLocked = listMutex.try_lock();
	listMutex.unlock();

	return !isLocked;
}
