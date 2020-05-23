#pragma once

#define QUEUE_DEBUG

#ifdef QUEUE_DEBUG
#undef NDEBUG
#endif
#include <assert.h>

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
		// checks that the array is not out of space
		assert(back % SIZE != front % SIZE || back == front);

		data[back++ % SIZE] = value;
	}

	T Dequeue()
	{
		// checks that there is data in the array
		assert(front != back);

		return data[front++ % SIZE];
	}

	T Peek()
	{
		// checks that there is data in the array
		assert(front != back);

		return data[front % SIZE];
	}

	bool IsEmpty()
	{
		return front == back;
	}

};

