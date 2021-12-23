#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool {
public:
	ThreadPool(size_t);
	template<class F, class... Args>
	auto enqueue(F&& f, Args&& ... args)
		->std::future<typename std::result_of<F(Args...)>::type>;
	~ThreadPool();
private:
	std::vector< std::thread > mWorkers;
	std::queue< std::function<void()> > mTasks;

	std::mutex mQueueMutex;
	std::condition_variable mCondition;
	bool mStop;
};

inline ThreadPool::ThreadPool(size_t threads)
	: mStop(false)
{
	for (size_t i = 0; i < threads; ++i)
		mWorkers.emplace_back(
			[this]
			{
				for (;;)
				{
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lock(this->mQueueMutex);
						this->mCondition.wait(lock,
							[this] { return this->mStop || !this->mTasks.empty(); });
						if (this->mStop && this->mTasks.empty())
							return;
						task = std::move(this->mTasks.front());
						this->mTasks.pop();
					}
					task();
				}
			}
			);
}

template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&& ... args)
-> std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;

	auto task = std::make_shared< std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(mQueueMutex);

		if (mStop)
			throw std::runtime_error("enqueue on stopped ThreadPool");

		mTasks.emplace([task]() { (*task)(); });
	}
	mCondition.notify_one();
	return res;
}

inline ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(mQueueMutex);
		mStop = true;
	}
	mCondition.notify_all();
	for (std::thread& worker : mWorkers)
		worker.join();
}