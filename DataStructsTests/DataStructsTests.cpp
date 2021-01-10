#pragma comment(lib, "Ws2_32.lib")
#define _CRT_SECURE_NO_WARNINGS 1
#include "../DataStructures/HashMap.h"
#include "../DataStructures/LinkedList.h"
#include "../DataStructures/Queue.h"

#include <iostream>
#include <string>
using namespace std;

typedef struct SomeStruct {
	int  number;
	char someChar;
}SS;

void TestQueue()
{
	Queue<int> numbersQueue;

	Queue<string> stringQueue(2);

	stringQueue.Enqueue(string("meda"));
	stringQueue.Enqueue(string("zekaa"));
	stringQueue.Enqueue(string("peka"));
	string front;
	stringQueue.GetFront(&front);

	cout << "Testing Enqueue more than size values, front expected 'meda', got " << front << endl;
	cout << "Testing more than size queue size. Expected 2 got " << stringQueue.Size() << endl;

	stringQueue.Dequeue();
	stringQueue.Dequeue();
	front = string("nothing");
	stringQueue.GetFront(&front);
	cout << "Testing Dequeue all, front expected 'nothing', got " << front << endl;
}

void TestHashMap()
{
	HashMap<SomeStruct> testMap;

	cout << "Testing Hash map does key exist on non existing key. Expected 0 got " << testMap.DoesKeyExist("TestKey") << endl;
	SomeStruct structic;
	structic.number = 555333;
	structic.someChar = 'k';

	testMap.Insert("Somekey", structic);
	cout << "Testing Hash map does key exist on  existing key. Expected 1 got " << testMap.DoesKeyExist("Somekey") << endl;

	SomeStruct retStruct;
	cout << "Testing get exisitng struct from hash map, expected 1 got " << testMap.Get("Somekey", &retStruct) << endl;
	cout << "Testing get exisitng struct from hash map, expected 555333 got " << retStruct.number << endl;
	cout << "Testing get exisitng struct from hash map, expected k got " << retStruct.someChar << endl;

	testMap.Delete("somekey");
	testMap.Delete("Somekey");
	cout << "Testing Hash map does key exist on deleted key. Expected 0 got " << testMap.DoesKeyExist("Somekey") << endl;

	HashMap<SomeStruct> testMapSmall(2);

	testMapSmall.Insert("key", structic);
	structic.number = 99899;
	structic.someChar = 'p';
	testMapSmall.Insert("key", structic);
	testMapSmall.Get("key", &retStruct);
	cout << "Testing get ovewritten struct from hash map, expected 99899 got " << retStruct.number << endl;
	cout << "Testing get overwritten struct from hash map, expected p got " << retStruct.someChar << endl;


	SomeStruct strkt;
	strkt.number = 0;
	strkt.someChar = 'a';
	testMapSmall.Insert("key0", strkt);

	strkt.number = 1;
	strkt.someChar = 'b';
	testMapSmall.Insert("key1", strkt);

	strkt.number = 2;
	strkt.someChar = 'c';
	testMapSmall.Insert("key2", strkt);
	
	strkt.number = 3;
	strkt.someChar = 'd';
	testMapSmall.Insert("key3", strkt);

	strkt.number = 4;
	strkt.someChar = 'e';
	testMapSmall.Insert("key4", strkt);

	testMapSmall.Get("key4", &retStruct);
	cout << "Testing get chained struct from hash map, expected 4 got " << retStruct.number << endl;
	cout << "Testing get chained struct  from hash map, expected e got " << retStruct.someChar << endl;

	testMapSmall.Delete("key");
	cout << "Testing Hash map does key exist on deleted key. Expected 0 got " << testMap.DoesKeyExist("key") << endl;

	testMapSmall.Delete("key4");
	cout << "Testing Hash map does key exist on deleted key. Expected 0 got " << testMap.DoesKeyExist("key4") << endl;

	testMapSmall.Delete("key1");
	testMapSmall.Delete("key2");

	testMapSmall.Insert("key5", strkt);

	

}

void TestLinkedList()
{
	LinkedList<int> list;
	int ret;

	cout << "Testing if list is empty on empty list expected 1 got" << list.isEmpty() << endl;
	cout << "Testing pop back on empty list. expected 0 got "  << list.PopBack(&ret) << endl;
	cout << "Testing pop front on empty list. expected 0 got " << list.PopFront(&ret) << endl;

	list.PushFront(5);
	list.PushFront(6);
	list.PushBack(4);
	list.PopBack(&ret);
	cout << "Testing pop back on list. expected  4 got " <<  ret << endl;
	list.PopFront(&ret);
	cout << "Testing pop front on list. expected  6 got " << ret << endl;
	list.PopBack(&ret);
	cout << "Testing pop back on list. expected  5 got " << ret << endl;
	cout << "Testing if list is empty on empty list expected 1 got" << list.isEmpty() << endl;

	for (int i = 10; i >= 0; i--)
	{
		list.PushFront(i);
	}

	int ackRes;
	ListNode<int> itBack = list.AcquireIteratorNodeBack(&ackRes);

	while (true)
	{
		if (itBack.GetValue() == 3)
		{
			cout << "Testing iterator found value!" << endl;
			break;
		}
		itBack = itBack.Previous();
	}

	list.ReleaseIterator();
	list.PushFront(199);
	list.PopFront(&ret);
	cout << "Testing iterator release and push after, expected 199 got " << ret << endl;

	ListNode<int> itFront = list.AcquireIteratorNodeFront(&ackRes);

	while (true)
	{
		if (itFront.GetValue() == 10)
		{
			cout << "Testing iterator found value!" << endl;
			break;
		}
		itFront = itFront.Next();
	}
	list.ReleaseIterator();

}


int main()
{
	TestQueue();
	TestHashMap();
	TestLinkedList();
}

