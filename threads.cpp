#include "threads.h"
#include <iostream>
#include <assert.h>

ThreadPool::ThreadPool(int threads, int maxCap, bool bStart)
	:_tasks(maxCap), _threads(threads)
{
	if (bStart)
	{
		start();
	}
}
ThreadPool::~ThreadPool()
{
	assert(_tasks.exited());
	if (_tasks.size())
	{
		std::cout<<_tasks.size()<<" tasks not processed"<<std::endl;
	}
}
void ThreadPool::start()
{
	for (auto& th : _threads)
	{
		std::thread t(
				[this]{
				while (!_tasks.exited())
				{
				Task task;
				if (_tasks.pop_wait(&task))
				{
				task();
				}
				}
				}
				);
		th.swap(t);
	}
}
void ThreadPool::join()
{
	for (auto& t: _threads)
	{
		t.join();
	}
}
bool ThreadPool::addTask(Task&& task)
{
	return _tasks.push(std::move(task));
}
