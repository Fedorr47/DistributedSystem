#include <iostream>
#include <thread>
#include <chrono>
#include <time.h>
#include <cstdlib>

#include "ThreadSafeQueue.h"
#include "ThreadPool.h"
#include "FunctionWrapper.h"

int WorkerFunction(double InDouble)
{
    std::cout << "Test print double = " << InDouble << std::endl;
    return static_cast<int>(InDouble + 10);
}

double AnotherWorkerFunction(int InInteger)
{
    std::cout << "Test print integer = " << InInteger << std::endl;
    return static_cast<double>(InInteger + 10);
}

int main(int argc, const char* argv[]) {

    FunctionWrapper lFunctionWrapper;
    lFunctionWrapper.Bind(&WorkerFunction, 10.0f);
    lFunctionWrapper();
    
    ThreadPool lThreadPool(std::thread::hardware_concurrency());
    std::future<int> futureInt = lThreadPool.enqueue(&WorkerFunction, 10.0f);
    std::future<double> futureDouble = lThreadPool.enqueue(&AnotherWorkerFunction, 10);
    std::cout << "Future results = " << futureInt.get() << " / " << futureDouble.get() << std::endl;

    return 0;
}