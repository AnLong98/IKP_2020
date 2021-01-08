#include "../DataStructures/HashMap.h"


// Constructor to initialize map
template <class T>
HashMap<T>::HashMap(unsigned int size)
{
	nodes = (HashMapNode<T>*) malloc(size * sizeof(HashMapNode<T>));
	for (int i = 0; i < size; i++)
	{
		nodes[i].next == NULL;
		nodes[i].key = NULL;
	}
		
	this->size = size;
	countUnique = 0;
}


//Function for string hashing, polynomial rolling hash
template <class T>
long long HashMap<T>::GetHash(const char* str)
{
	// P and M
	int p = 53; // english alphabet upper and lowercase
	int m = 1e9 + 9; //very large number to prevent collision
	long long powerOfP = 1;
	long long hashVal = 0;

	for (int i = 0; i < strlen(str); i++) 
	{
		hashVal = (hashVal + (str[i] - 'a' + 1) * powerOfP)% m;
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
				
			
		} while (node->next != NULL);
	}

	mapMutex.unlock();
	return false;
	
}