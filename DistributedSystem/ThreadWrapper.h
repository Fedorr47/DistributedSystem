#pragma once

#include <future>
#include <thread>
#include <functional>

class InterruptedThread : public std::exception
{
private:
	std::string mWhat;
public:
	InterruptedThread(
		std::string InWhat
	) : mWhat(InWhat)
	{
	}
	virtual const char* what() const noexcept
	{
		return mWhat.c_str();
	}
};

class InterruptFlag
{
	std::atomic<bool> mFlag;
	std::condition_variable* mThreadCond;
	std::mutex mSetClearMutex;

public:
	InterruptFlag() :
		mThreadCond(0)
	{}

	void Set()
	{
		mFlag.store(true, std::memory_order_relaxed);
		std::lock_guard<std::mutex> lk(mSetClearMutex);
		if (mThreadCond)
		{
			mThreadCond->notify_all();
		}
	}

	bool IsSet() const
	{
		return mFlag.load(std::memory_order_relaxed);
	}

	void SetConditionVariable(std::condition_variable& InConditionalVariable)
	{
		std::lock_guard<std::mutex> lk(mSetClearMutex);
		mThreadCond = &InConditionalVariable;
	}

	void ClearConditionVariable()
	{
		std::lock_guard<std::mutex> lk(mSetClearMutex);
		mThreadCond = 0;
	}

	struct clear_cv_on_destruct
	{
		~clear_cv_on_destruct();
	};
};

thread_local InterruptFlag ThisThreadInterruptFlag;
InterruptFlag::clear_cv_on_destruct::~clear_cv_on_destruct()
{
	ThisThreadInterruptFlag.ClearConditionVariable();
}

void InterruptionPoint()
{
	if (ThisThreadInterruptFlag.IsSet())
	{
		throw InterruptedThread("Interupt");
	}
}

template<class _Predicate>
void InterruptibleWait(
	std::condition_variable& InConditionalVariable,
	std::unique_lock<std::mutex>& InLock,
	_Predicate predic = nullptr
)
{
	InterruptionPoint();
	ThisThreadInterruptFlag.SetConditionVariable(InConditionalVariable);
	InterruptFlag::clear_cv_on_destruct guard;
	InterruptionPoint();
	InConditionalVariable.wait_for(InLock, std::chrono::milliseconds(1));
	InterruptionPoint();
}

/// <summary>
/// ThreadWrapper - wrapper to thread that can be run and interrupted
/// </summary>
class ThreadWrapper
{
private:
	std::promise<void> mPromiseResult;
	std::future<void> mResult;
	std::thread mThread;
	std::function<void()> mCallbackFunction;
	std::function<void()> mCallbackException;
	InterruptFlag* mFlag;

	void ThreadFunction(
		std::promise<void>& promise_result,
		std::promise<InterruptFlag*>&
	);

public:
	template<typename Callable, typename... Args>
	explicit
		ThreadWrapper(Callable&& callbackFunc, Args&& ... args)
	{
		mCallbackFunction = std::bind(callbackFunc, args...);
	}
	~ThreadWrapper()
	{
		if (mThread.joinable())
			mThread.join();
	}
	template<typename Callable, typename... Args>
	void set_callback_exception(Callable&& callbackFunc, Args&& ... args)
	{
		mCallbackException = std::bind(callbackFunc, args...);
	}
	void Interrupt()
	{
		if (mFlag)
		{
			mFlag->Set();
		}
	}

	bool IsFinish();
	void Run();
	void JoinThread();
	void Get();
};


void ThreadWrapper::Run()
{
	mPromiseResult = std::promise<void>();
	mResult = mPromiseResult.get_future();
	std::promise<InterruptFlag*> p;
	mThread = std::thread(
		&ThreadWrapper::ThreadFunction,
		this,
		std::ref(mPromiseResult),
		std::ref(p)
	);
	mFlag = p.get_future().get();
}

void ThreadWrapper::ThreadFunction(
	std::promise<void>& promise_result,
	std::promise<InterruptFlag*>& p
)
{
	try
	{
		p.set_value(reinterpret_cast<InterruptFlag*>(&ThisThreadInterruptFlag));
		this->mCallbackFunction();
	}
	catch (const std::exception& ex)
	{
		promise_result.set_exception(std::current_exception());
		if (mCallbackException)
			mCallbackException();
		return;
	}
	catch (...)
	{
		promise_result.set_exception(std::current_exception());
		if (mCallbackException)
			mCallbackException();
		return;
	}
	promise_result.set_value();
}

void ThreadWrapper::JoinThread()
{
	mThread.join();
	Get();
}

void ThreadWrapper::Get()
{
	mResult.get();
}

bool ThreadWrapper::IsFinish()
{
	return mThread.joinable();
}
