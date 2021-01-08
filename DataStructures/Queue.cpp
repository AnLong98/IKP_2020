#include "../DataStructures/Queue.h"

// Constructor to initialize queue
template <class T>
Queue<T>::Queue(int size)
{
	elements = new T[size];
	capacity = size;
	front = 0;
	rear = -1;
	count = 0;
}

//Destructor to destroy queue
template <class T>
Queue<T>::~Queue()
{
	delete[] elements;
}

// Utility function to remove front element from the queue
template <class T>
bool Queue<T>::Dequeue()
{
	queueMutex.lock();
	if (isEmpty())
	{
		queueMutex.unlock();
		return false;
	}

	front = (front + 1) % capacity;
	count--;
	queueMutex.unlock();
	return true;
}

// Utility function to add an item to the queue
template <class T>
bool Queue<T>::Enqueue(T value)
{
	queueMutex.lock();
	if (isFull())
	{
		queueMutex.unlock();
		return false;
	}

	rear = (rear + 1) % capacity;
	elements[rear] = value;
	count++;
	queueMutex.unlock();
	return true;
}

// Utility function to return front element in the queue
template <class T>
bool Queue<T>::GetFront(T* value)
{
	queueMutex.lock();
	if (isEmpty())
	{
		queueMutex.unlock();
		return false;
	}
	*value = elements[front];
	queueMutex.unlock();
	return true;
}

// Utility function to return the size of the queue
template <class T>
int Queue<T>::Size()
{
	return count;
}

// Utility function to check if the queue is empty 
template <class T>
bool Queue<T>::isEmpty()
{
	return (Size() == 0);
}

// Utility function to check if the queue is full
template <class T>
bool Queue<T>::isFull()
{
	return (Size() == capacity);
}
