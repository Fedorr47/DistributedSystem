#pragma once

#include <functional>
#include <memory>

class FunctionWrapper
{	
	std::function<void()> mInternalFunction;
public:

	FunctionWrapper() = default;

	template<class F, class... Args>
	auto Bind(F&& f, Args&& ... args) -> std::future<typename std::result_of<F(Args...)>::type>
	{
		using return_type = typename std::result_of<F(Args...)>::type;

		auto task = std::make_shared< std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

		std::future<return_type> res = task->get_future();
		{
			mInternalFunction = [task]() { (*task)(); };
		}

		return res;
	}

	void operator()()
	{
		mInternalFunction();
	}

	FunctionWrapper(FunctionWrapper&& Other) noexcept :
		mInternalFunction(std::move(Other.mInternalFunction))
	{}

	FunctionWrapper& operator=(FunctionWrapper&& Other) noexcept
	{
		mInternalFunction = std::move(Other.mInternalFunction);
		return *this;
	}

	FunctionWrapper(FunctionWrapper& Other) = delete;
	FunctionWrapper(const FunctionWrapper& Other) = delete;
	FunctionWrapper& operator=(const FunctionWrapper& Other) = delete;
};