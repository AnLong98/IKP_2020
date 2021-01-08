#include "../DataStructures/LinkedList.h"

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
LinkedList<T>::LinkedList(int size)
{
	head = (T*)malloc(size * sizeof(T));
	count = 0;
	rear = head;

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
		ListNode<T> newNode = new ListNode<T>();
		newNode.value = element;
		newNode.previous = NULL;
		newNode.next = NULL;
		head = &newNode;
		rear = &newNode;
		count++;
		listMutex.unlock();
		return true;
	}
	ListNode<T> newNode = new ListNode<T>();
	newNode.value = element;
	newNode.previous = NULL;
	newNode.next = head;
	head = &newNode;
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
		ListNode<T> newNode = new ListNode<T>();
		newNode.value = element;
		newNode.previous = NULL;
		newNode.next = NULL;
		head = &newNode;
		rear = &newNode;
		count++;
		listMutex.unlock();
		return true;
	}
	ListNode<T> newNode = new ListNode<T>();
	newNode.value = element;
	newNode.previous = rear;
	newNode.next = NULL;
	rear = &newNode;
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
		head = NULL;
		rear = NULL;
	}
	else
	{
		head = frontNode->next;
		head->previous = NULL;
	}
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
		head = NULL;
		rear = NULL;
	}
	else
	{
		rear = backNode->previous;
		rear->next = NULL;
	}
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
			head = NULL;
			rear = NULL;
			count--;
			delete search;
			listMutex.unlock();
			return true;
		}
		else if (search->next == node.next && search->previous == node.previous)
		{
			(search->previous)->next = search->next;
			(search->next)->previous = search->previous;
			count--;
			delete search;
			listMutex.unlock();
			return true;
		}

		search = search->next;
	}
	listMutex.unlock();
	return false;
}

// Utility function to remove list element
template <class T>
ListNode<T> LinkedList<T>::AcquireIteratorNodeBack()
{
	listMutex.lock();
	if (rear != NULL)
	{
		return *rear;
	}

	return NULL;
}

// Utility function to remove list element
template <class T>
ListNode<T> LinkedList<T>::AcquireIteratorNodeFront()
{
	listMutex.lock();
	if (head != NULL)
	{
		return *head;
	}

	return NULL;
}

// Utility function to remove list element
template <class T>
bool LinkedList<T>::ReleaseIterator()
{
	bool isLocked = listMutex.try_lock();
	listMutex.unlock();

	return !isLocked;
}
