#include "../DataStructures/HashMap.h"

template <class T>
HashMapNode<T>::HashMapNode(char* key, T value)
{
	this->key = (char*)malloc(strlen(key));
	strcpy(this->key, key);
	this->value = value;
	this->next = NULL;
}

template <class T>
HashMapNode<T>::HashMapNode()
{
	this->key = NULL;
	this->next = NULL;
}

template <class T>
HashMapNode<T>::~HashMapNode()
{
	if(this->key != NULL)
		free(this->key);
	if (this->next != NULL)
		delete this->next;
}


// Constructor to initialize map
template <class T>
HashMap<T>::HashMap(unsigned int size)
{
	nodes = new HashMapNode<T>[size];
	for (int i = 0; i < size; i++)
	{
		nodes[i].next == NULL;
		nodes[i].key = NULL;
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
		hashVal = (hashVal + (key[i] - 'a' + 1) * powerOfP)% m;
		powerOfP = (powerOfP * p) % m;
	}
	return hashVal;
}

//Function for string hashing, polynomial rolling hash
template <class T>
bool HashMap<T>::DoesKeyExist(char* key)
{
	if (countUnique == 0)return false;

	long long hashValue = GetHash(key);

	mapMutex.lock();
	int index = hashValue % this->size;
	if (nodes[index].key == NULL)
	{
		mapMutex.unlock();
		return false;
	}
	else
	{
		HashMapNode<T> node = nodes[index];
		do
		{
			if (strcmp(node->key, key) == 0)
			{
				mapMutex.unlock();
				return true;
			}
				
			node = node->next;
		} while (node != NULL);
	}

	mapMutex.unlock();
	return false;
	
}

//Function for inserting into hash map
template <class T>
void HashMap<T>::Insert(char* key, T value)
{
	long long hashValue = GetHash(key);

	mapMutex.lock();
	int index = hashValue % this->size;
	if (nodes[index].key == NULL)
	{
		nodes[index].key = (char*)malloc(strlen(key));
		strcpy(nodes[index].key, key);
		nodes[index].value = value;
		mapMutex.unlock();
		return;
	}
	else
	{
		HashMapNode<T>* node = nodes + index;
		while (node->next != NULL);
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
			HashMapNode<T>* newNode =  new HashMapNode<T>(key, value);
			node->next = newNode;
			countUnique++;
		}
	}

	mapMutex.unlock();
	return;
}


//Function for getting value from hash map
template <class T>
bool HashMap<T>::Get(char* key, T* value)
{
	if (countUnique == 0)return false;

	long long hashValue = GetHash(key);

	mapMutex.lock();
	int index = hashValue % this->size;
	if (nodes[index].key == NULL)
	{
		mapMutex.unlock();
		return false;
	}
	else
	{
		HashMapNode<T> node = nodes[index];
		do
		{
			if (strcmp(node->key, key) == 0)
			{
				*value = node.value;
				mapMutex.unlock();
				return true;
			}

			node = node->next;
		} while (node != NULL);
	}

	mapMutex.unlock();
	return false;
}


//Function to delete element from hash map
template <class T>
void HashMap<T>::Delete(char* key)
{
	if (countUnique == 0)return;

	long long hashValue = GetHash(key);

	mapMutex.lock();
	int index = hashValue % this->size;
	if (nodes[index].key == NULL)
	{
		mapMutex.unlock();
		return;
	}

	HashMapNode<T>* node = nodes + index;
	if (strcmp(node->key, key) == 0 && node->next == NULL) //if first element is to be deleted and there are no chained collisions just set it to null
	{
		free(node->key);
		node->key = NULL;
		mapMutex.unlock();
		return;
	}
	else if (strcmp(node->key, key) == 0 && node->next != NULL)//if first element is to be deleted and there are chained collisions move last to it's spot
	{
		HashMapNode<T>* nodeNext = node->next;
		while (nodeNext->next != NULL)
		{
			node = node->next;
			nodeNext = nodeNext->next;
		}

		node->next = NULL; //Set previous to NULL
		HashMapNode<T>* first = nodes + index;
		first->value = nodeNext->value; //Copy last's value to first
		free(first->key); 
		first->key = (char*)malloc(strlen(nodeNext->key));
		strcpy(first->key, nodeNext->key);//Store last's key in first's
		delete nodeNext; //delete last
		mapMutex.unlock();
		return;
	}

	HashMapNode<T>* nodeNext = node->next;
	do
	{
		if (strcmp(nodeNext->key, key) == 0)
		{
			node->next = nodeNext->next;
			nodeNext->next = NULL;
			delete nodeNext;
			mapMutex.unlock();
			return true;
		}

		node = node->next;
		nodeNext = nodeNext->next;
	} while (nodeNext != NULL);

	mapMutex.unlock();
	return false;
}