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