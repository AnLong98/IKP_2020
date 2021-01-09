#pragma once
#include <iostream>
#include <cstdlib>
#include <mutex>
#define _CRT_SECURE_NO_WARNINGS 1
#define INITIAL_MAP_SIZE 50
using namespace std;

template <class T>
class HashMapNode
{
private:
	char* key;
	T value;
	HashMapNode* next;
	template <class T> friend class HashMap;

public:
	HashMapNode(char* key, T value);		//ctor
	HashMapNode();							//ctor
	~HashMapNode();							//DTOR

};

//Hash map that maps char* strings to generic values
template <class T>
class HashMap
{
private:
	HashMapNode<T>* nodes;					//Nodes array
	mutex mapMutex;							//Mutex used for map access
	unsigned int size;						//Hash map maximum unique key spots without chained nodes
	unsigned int countUnique;				//Current number of unique key spots in use
	long long GetHash(const char* str);		//Hash function for string hashing

public:
	HashMap(unsigned int size = INITIAL_MAP_SIZE);		//CTOR
	~HashMap();											//DTOR
	void Insert(const char* key, T value);					//Method for inserting a value in hash map, replaces existing one with the same key
	void Delete(const char* key);								//Method for deleting a value in hash map, returns false if not found
	bool Get(const char* key, T* value);						//Method for getting value by key, value is stored in value pointer, returns false if key doesn't exist
	bool DoesKeyExist(const char* key);						//Checks if key exists in hash map
};

template <class T>
HashMapNode<T>::HashMapNode(char* key, T value)
{
	this->key = (char*)malloc(strlen(key) + 1);
	strcpy(this->key, key);
	this->value = value;
	this->next = nullptr;
}

template <class T>
HashMapNode<T>::HashMapNode()
{
	this->key = nullptr;
	this->next = nullptr;
}

template <class T>
HashMapNode<T>::~HashMapNode()
{
	if (this->key != nullptr)
		free(this->key);
	if (this->next != nullptr)
		delete this->next;
}


// Constructor to initialize map
template <class T>
HashMap<T>::HashMap(unsigned int size)
{
	nodes = new HashMapNode<T>[size];
	for (int i = 0; i < size; i++)
	{
		nodes[i].next = nullptr;
		nodes[i].key = nullptr;
	}

	this->size = size;
	countUnique = 0;
}

// Destructor
template <class T>
HashMap<T>::~HashMap()
{
	delete[] nodes;
}


//Function for string hashing, polynomial rolling hash
template <class T>
long long HashMap<T>::GetHash(const char* key)
{
	// P and M
	int p = 53; // english alphabet upper and lowercase
	int m = 1e9 + 9; //very large number to prevent collision
	long long powerOfP = 1;
	long long hashVal = 0;

	for (int i = 0; i < strlen(key); i++)
	{
		hashVal = (hashVal + (key[i] - 'a' + 1) * powerOfP) % m;
		powerOfP = (powerOfP * p) % m;
	}
	return hashVal;
}

//Function for checking if key exists in hash map
template <class T>
bool HashMap<T>::DoesKeyExist(const char* key)
{
	if (countUnique == 0)return false;

	unsigned long long hashValue = GetHash(key);

	mapMutex.lock();
	unsigned int index = hashValue % this->size;
	if (nodes[index].key == nullptr)
	{
		mapMutex.unlock();
		return false;
	}
	else
	{
		HashMapNode<T>* node = nodes + index;
		do
		{
			if (strcmp(node->key, key) == 0)
			{
				mapMutex.unlock();
				return true;
			}

			node = node->next;
		} while (node != nullptr);
	}

	mapMutex.unlock();
	return false;

}

//Function for inserting into hash map, if value exists it will be replaced
template <class T>
void HashMap<T>::Insert(const char* key, T value)
{
	unsigned long long hashValue = GetHash(key);

	mapMutex.lock();
	int index = hashValue % this->size;
	if (nodes[index].key == nullptr)
	{
		nodes[index].key = (char*)malloc(strlen(key) + 1);
		strcpy(nodes[index].key, key);
		nodes[index].value = value;
		countUnique++;
		mapMutex.unlock();
		return;
	}
	else
	{
		HashMapNode<T>* node = nodes + index;
		while ((node->next) != nullptr)
		{
			node = node->next;
		}

		if (strcmp(node->key, key) == 0) //Replace existing
		{
			node->value = value;
			mapMutex.unlock();
			return;
		}
		else //Add new
		{
			char* keyCopy = _strdup(key);
			HashMapNode<T>* newNode = new HashMapNode<T>(keyCopy, value);
			node->next = newNode;
			countUnique++;
			free(keyCopy);
		}
	}

	mapMutex.unlock();
	return;
}


//Function for getting value from hash map
template <class T>
bool HashMap<T>::Get(const char* key, T* value)
{
	if (countUnique == 0)return false;

	unsigned  long long hashValue = GetHash(key);

	mapMutex.lock();
	unsigned int index = hashValue % this->size;
	if (nodes[index].key == nullptr)
	{
		mapMutex.unlock();
		return false;
	}
	else
	{
		HashMapNode<T>* node = nodes + index;
		do
		{
			if (strcmp(node->key, key) == 0)
			{
				*value = node->value;
				mapMutex.unlock();
				return true;
			}

			node = node->next;
		} while (node != nullptr);
	}

	mapMutex.unlock();
	return false;
}


//Function to delete element from hash map
template <class T>
void HashMap<T>::Delete(const char* key)
{
	if (countUnique == 0)return;

	unsigned long long hashValue = GetHash(key);

	mapMutex.lock();
	unsigned int index = hashValue % this->size;
	if (nodes[index].key == nullptr)
	{
		mapMutex.unlock();
		return;
	}

	HashMapNode<T>* node = nodes + index;
	if (strcmp(node->key, key) == 0 && node->next == nullptr) //if first element is to be deleted and there are no chained collisions just set it to null
	{
		free(node->key);
		node->key = nullptr;
		countUnique--;
		mapMutex.unlock();
		return;
	}
	else if (strcmp(node->key, key) == 0 && node->next != nullptr)//if first element is to be deleted and there are chained collisions move last to it's spot
	{
		HashMapNode<T>* nodeNext = node->next;
		while (nodeNext->next != nullptr)
		{
			node = node->next;
			nodeNext = nodeNext->next;
		}

		node->next = nullptr; //Set previous to nullptr
		HashMapNode<T>* first = nodes + index;
		first->value = nodeNext->value; //Copy last's value to first
		free(first->key);
		first->key = (char*)malloc(strlen(nodeNext->key) + 1);
		strcpy(first->key, nodeNext->key);//Store last's key in first's
		delete nodeNext; //delete last
		countUnique--;
		mapMutex.unlock();
		return;
	}

	HashMapNode<T>* nodeNext = node->next;
	do
	{
		if (strcmp(nodeNext->key, key) == 0)
		{
			node->next = nodeNext->next;
			nodeNext->next = nullptr;
			delete nodeNext;
			countUnique--;
			mapMutex.unlock();
			return;
		}

		node = node->next;
		nodeNext = nodeNext->next;
	} while (nodeNext != nullptr);

	mapMutex.unlock();
	return;
}