#pragma once
#include <iostream>
#include <cstdlib>
#include <mutex>
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
};

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
	void Insert(char* key, T value);					//Method for inserting a value in hash map, replaces existing one with the same key
	bool Get(char* key, T* value);						//Method for getting value by key, value is stored in value pointer, returns false if key doesn't exist
	bool DoesKeyExist(char* key);						//Checks if key exists in hash map
};