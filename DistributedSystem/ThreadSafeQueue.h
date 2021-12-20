#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>

template <class T>
class ThreadSafeQueue
{
private:
	using SharedType = std::shared_ptr<T>;
	using LockGuardMutex = std::lock_guard<std::mutex>;
	using UniqueLock = std::unique_lock<std::mutex>;

	struct Node
	{
		SharedType data;
		std::unique_ptr<Node> next;
	};

	using UniqueNode = std::unique_ptr<Node>;
	
	std::mutex mHeadMutex, mTailMutex;
	std::atomic<Node*> mHead;
	std::atomic<Node*> mTail;
	std::condition_variable mDataCond;

	Node* GetTail()
	{
		LockGuardMutex lTailLock(mTailMutex);
		return mTail;
	}

	UniqueNode PopHead()
	{
		UniqueNode const lOldHead = std::move(mHead);
		mHead = std::move(mHead->next);
		return lOldHead;
	}

	UniqueLock WaitForData()
	{
		UniqueLock lHeadLock(mHeadMutex);
		mDataCond.wait(lHeadLock, [&](){return mHead != GetTail(); });
		return std::move(lHeadLock);
	}

	UniqueNode WaitPopHead()
	{
		UniqueLock lHeadLock(WaitForData());
		return PopHead();
	}

	UniqueNode WaitPopHead(T& OutValue)
	{
		UniqueLock lHeadLock(WaitForData());
		OutValue = std::move(*mHead->data);
		return PopHead();
	}

	UniqueNode TryPopHead()
	{
		LockGuardMutex lHeadLock(mHeadMutex);
		if (mHead.get() == GetTail())
		{
			return UniqueNode();
		}
		return PopHead();
	}

	UniqueNode TryPopHead(T& OutValue)
	{
		LockGuardMutex lHeadLock(mHeadMutex);
		if (mHead.get() == GetTail())
		{
			return UniqueNode();
		}
		OutValue = std::move(*mHead->data);
		return PopHead();
	}

public:

	ThreadSafeQueue() :
		mHead(new Node)
	{
		mTail.store(mHead.load());
	}

	ThreadSafeQueue(const ThreadSafeQueue& Other) = delete;
	ThreadSafeQueue& operator=(const ThreadSafeQueue& Other) = delete;

	void Push(T InValue)
	{
		SharedType NewData = std::make_shared<T>(std::move(InValue));
		UniqueNode NextNode(new Node);
		{
			LockGuardMutex lTailLock(mTailMutex);
			mTail->data = InValue;
			Node* const lNewTail = NextNode.get();
			mTail->next = std::move(NextNode);
			mTail = lNewTail;
		}
		mDataCond.notify_one();
	}

	SharedType WaitAndPop()
	{
		UniqueNode const lOldHead = WaitPopHead();
		return lOldHead->data;
	}

	void WaitAndPop(T& OutValue)
	{
		UniqueNode const lOldHead = WaitPopHead(OutValue);
	}

	SharedType TryPop()
	{
		UniqueNode const lOldHead = TryPopHead();
		return lOldHead ? lOldHead->data : SharedType();
	}
	
	bool TryPop(T& OutValue)
	{
		UniqueNode const lOldHead = TryPopHead(OutValue);
		return lOldHead;
	}

	bool Empty()
	{
		LockGuardMutex lHeadLock(mHeadMutex);
		return (mHead == GetTail());
	}
};