#pragma once

#define QUEUE_DEBUG

#ifdef QUEUE_DEBUG
#undef NDEBUG
#include <assert.h>
#endif

#define SIZE 256

template <typename T>
class Queue
{
private:

	int front;
	int back;
	T data[SIZE];

public:

	Queue()
		:
		front(0), back(0)
	{
	}

	void Enqueue(T value)
	{
#ifdef QUEUE_DEBUG

		// checks that the array is not out of space
		assert(back % SIZE != front % SIZE || back == front);
#endif

		data[back++ % SIZE] = value;
	}

	T Dequeue()
	{
#ifdef QUEUE_DEBUG

		// checks that there is data in the array
		assert(front != back);
#endif

		return data[front++ % SIZE];
	}

	T Peek()
	{
#ifdef QUEUE_DEBUG

		// checks that there is data in the array
		assert(front != back);
#endif

		return data[front % SIZE];
	}

	bool IsEmpty()
	{
		return front == back;
	}

};

