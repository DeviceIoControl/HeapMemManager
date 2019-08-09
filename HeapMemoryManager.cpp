//============================================================================
// Name        : WAPSpoof.cpp
// Author      : Josh Stephenson
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <atomic>
#include <pthread.h>
#include <unistd.h>

using byte = unsigned char;

class spinlock
{
public:
	spinlock() : locked(false)
	{

	}

	inline void lock()
	{
		if(locked)
		{
			this->spin();
		}
		else
		{
			locked = true;
			thread_id = (unsigned int)pthread_self();
		}
	}

	inline bool release()
	{
		if((unsigned int)pthread_self() != thread_id)
		{
			return false;
		}

		locked = false;
		return true;
	}

	~spinlock()
	{
		locked = false;
	}

private:
	bool locked;
	unsigned int thread_id;

	inline void spin()
	{
		while(locked);
		this->lock();
	}
};


template<typename T>
struct Stack
{
	T* Top;
	T* Base;
};

template<typename T>
struct HeapInfo
{
	unsigned int bytesRemaining;
	byte freedCookie;
	Stack<T> allocStack;
	unsigned int lastFreedOffset;
};

};

void* pHeapMemory = calloc((256 * 1024) * 1024, 1);

class Object
{
public:
	Object()
	{
	
	}

	inline static void* operator new(const unsigned int size)
	{
		spinlock sl;
		sl.lock();

		HeapInfo<Object>* pHeapInfo = reinterpret_cast<HeapInfo<Object>*>(pHeapMemory);

		std::cout << "	    Heap Management Info: \n";
		std::cout << "-----------------------------------\n";

		if(pHeapInfo->allocStack.Top == 0 && pHeapInfo->allocStack.Base == 0)
		{
			pHeapInfo->allocStack.Base = reinterpret_cast<Object*>(((byte*)pHeapMemory + pHeapInfo->bytesRemaining));
			pHeapInfo->allocStack.Top = pHeapInfo->allocStack.Base;
		}

		std::cout << "Allocation Stack (Top): " << pHeapInfo->allocStack.Top << "\n";
		std::cout << "Allocation Stack (Base): " << pHeapInfo->allocStack.Base << "\n";
		std::cout << "Number of allocated objects: " << (unsigned int)(pHeapInfo->allocStack.Base - pHeapInfo->allocStack.Top) << "\n";
		std::cout << "Bytes remaining: " << pHeapInfo->bytesRemaining << "\n";

		if(pHeapInfo->lastFreedOffset)
		{
			Object* pLastFreed = &pHeapInfo->allocStack.Top[pHeapInfo->lastFreedOffset];
			std::cout << "Last Freed Object offset: " << pHeapInfo->lastFreedOffset << "\n";
			memset(pLastFreed, 0, size);
			std::cout << "Next allocation: " << pHeapInfo->allocStack.Top << "\n\n";
			return pLastFreed;
		}

		Object* pNextAlloc = pHeapInfo->allocStack.Top;

		if(pHeapInfo->allocStack.Top > (Object*)&pHeapInfo[1])
		{
			pHeapInfo->bytesRemaining -= size;
			pHeapInfo->allocStack.Top--;
		}

		std::cout << "Next allocation: " << (void*)pNextAlloc << "\n\n";
	 	sl.release();
		return pNextAlloc;
	}

	inline static void operator delete(void* pThis, const unsigned int size)
	{
		HeapInfo<Object>* pHeapInfo =  reinterpret_cast<HeapInfo<Object>*>(pHeapMemory);
		byte* max = (byte*)&pHeapInfo[1];
		byte* min = (byte*)pHeapInfo->allocStack.Base;

		if(!pHeapInfo->freedCookie){
			pHeapInfo->freedCookie = rand() % 0xFF;
		}

		if(((byte*)pThis) < min && ((byte*)pThis) > max)
		{
			pHeapInfo->lastFreedOffset = (((Object*)pThis) - pHeapInfo->allocStack.Top);
			memset(pThis, pHeapInfo->freedCookie, size);
		}

		if((Object*)pThis == pHeapInfo->allocStack.Top)
		{
			pHeapInfo->allocStack.Top++;
			pHeapInfo->bytesRemaining += size;
		}
	}

	~Object()
	{
	
	}

private:
	int a = 10;
	float f = 69.69f;
	void* p = this;
	byte abc[2];
};

int main(int argc, const char** argv)
{
	getchar();
	return 0;

	((HeapInfo<Object>*)pHeapMemory)->bytesRemaining = ((256 * 1024) * 1024);

	std::atomic<Object*> pObj(nullptr);

	void(* allocateThread)(std::atomic<Object*>&) = nullptr;

	allocateThread = [](std::atomic<Object*>& pObject) -> void
	{
		for(int i = 0; i < 1500000; ++i)
		{
			pObject = new Object();
		}
	};

	pthread_t threads[2] = { NULL };
	int threadHandles[2] = { NULL };

	void*( *castedFn)(void*) = (decltype(castedFn))allocateThread;
	threadHandles[0] = pthread_create(&threads[0], nullptr, castedFn, &pObj);
	threadHandles[1] = pthread_create(&threads[1], nullptr, castedFn, &pObj);

	for(int i = 0; i < 2; ++i)
	{
		pthread_join(threads[i], nullptr);
	}

	for(int i = 0; i < 2; ++i)
	{
		close(threadHandles[i]);
	}

	for(int i = 0; i < 10; ++i)
	{
		delete pObj;
		pObj = new Object();
	}

	getchar();
	return 1;
}
