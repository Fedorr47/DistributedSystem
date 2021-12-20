#pragma once

#include <functional>
#include <memory>

class FunctionWrapper
{	
	std::function<void()> mInternalFunction;
public:

	FunctionWrapper() = default;

	template<class Function, class... Args>
	std::future<typename std::result_of<Function(Args...)>::type> Bind(Function&& InFunc, Args&& ... args)
	{
		using ReturnType = typename std::result_of<Function(Args...)>::type;

		std::shared_ptr<std::packaged_task<ReturnType()>> lTask = std::make_shared< std::packaged_task<ReturnType()>>(
			std::bind(std::forward<Function>(InFunc), std::forward<Args>(args)...)
			);

		std::future<ReturnType> Result = lTask->get_future();
		{
			mInternalFunction = [lTask]() { (*lTask)(); };
		}

		return Result;
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