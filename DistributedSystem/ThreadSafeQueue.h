#pragma once

#include <memory>
#include <mutex>

template <class T>
class ThreadSafeQueue
{
private:
	struct Node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<Node> next;
	};

public:
};