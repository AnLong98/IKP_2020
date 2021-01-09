#include <iostream>
#include <cstdlib>
#include <mutex>
using namespace std;

// define default capacity of the queue
#define QUEUE_SIZE 30

// Class for queue
template <class T>
class Queue
{
private:
	mutex queueMutex;	  //Queue mutex
	T *elements;         // array to store queue elements
	int capacity;		// maximum capacity of the queue
	int front;			// front points to front element in the queue (if any)
	int rear;			// rear points to last element in the queue
	int count;			// current size of the queue

public:
	Queue(int size = QUEUE_SIZE);        //CTOR
	~Queue();							//DTOR
	bool Dequeue();						//Remove element from the queue front, returns false if queue is empty
	bool Enqueue(T value);				//Add value to the queue rear, returns false if queue is full
	bool GetFront(T* value);			//Get element from the front of the queue, returns true if queue is not empty
	int Size();							//Get queue size
	bool isEmpty();						//Check if empty
	bool isFull();						//Check if full
};

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
