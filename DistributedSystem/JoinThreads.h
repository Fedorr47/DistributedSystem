#pragma once

#include <vector>
#include <thread>

namespace ThreadTools
{
	class JoinThreads
	{
		std::vector<std::thread> mVecThreads;
	public:
		explicit JoinThreads(std::vector<std::thread>& InVecThreads) :
			mVecThreads(InVecThreads)
		{}
		~JoinThreads()
		{
			for (std::thread& JoinableThread : mVecThreads)
			{
				if (JoinableThread.joinable())
				{
					JoinableThread.join();
				}
			}
		}
	};
}