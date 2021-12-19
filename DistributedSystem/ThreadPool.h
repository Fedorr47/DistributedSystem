#pragma once

#include <atomic>

#include "JoinThreads.h"

namespace ThreadTools
{
	class ThreadPool
	{
	private:
		std::atomic_bool mDone;
		JoinThreads mJoiner;
	public:

	};
}