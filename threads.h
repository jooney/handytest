#ifndef __THREADS_H__
#define __THREADS_H__ 

#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <limits>
#include <list>
#include <condition_variable>
#include <functional>
#include "Util.h"

template<typename T> class WorkQueue : public std::mutex, public noncopyable 
{
	public:
		static const int wait_infinite = std::numeric_limits<int>::max();
		WorkQueue(size_t capacity = 0)
			:_capacity(capacity),_exit(false){}
		bool push(T&& v);
		T pop_wait(int waitMs = wait_infinite);
		bool pop_wait(T* v, int waitMs=wait_infinite);
		size_t size();
		void exit();  //notify all 
		bool exited(){return _exit;}
	private:
		std::list<T> _items;
		std::condition_variable _ready;
		size_t  _capacity;
		std::atomic<bool> _exit;
		void wait_ready(std::unique_lock<std::mutex>& lck, int waitMs);
};
template<typename T>
size_t WorkQueue<T>::size()
{
	std::lock_guard<std::mutex>lck(*this);
	return _items.size();
}
template<typename T>
void WorkQueue<T>::exit()
{
	_exit = true;
	std::lock_guard<std::mutex> lck(*this);
	_ready.notify_all();
}
template<typename T>
bool WorkQueue<T>::push(T&& v)
{
	std::lock_guard<std::mutex> lck(*this);
	if (_exit || (_capacity && _items.size() >= _capacity))
		return false;
	_items.push_back(std::move(v));
	_ready.notify_one();
	return true;
}
template<typename T>
void WorkQueue<T>::wait_ready(std::unique_lock<std::mutex>& lck, int waitMs)
{
	if (_exit || !_items.empty()) // _items is not empty , so we can get item 
		return ;
	if (waitMs == wait_infinite) // if no items ,just wait forever until we can get one item
	{
		_ready.wait(lck,[this]{return _exit || !_items.empty();});	
	}
	else if (waitMs > 0)
	{
		auto tp = std::chrono::steady_clock::now() + std::chrono::milliseconds(waitMs);
		while (_ready.wait_until(lck,tp) != std::cv_status::timeout && _items.empty() && !_exit)
		{}
	}
}
template<typename T>
bool WorkQueue<T>::pop_wait(T* v, int waitMS)
{
	std::unique_lock<std::mutex> lck(*this);
	wait_ready(lck,waitMS);
	if (_items.empty())
		return false;
	*v = std::move(_items.front());
	_items.pop_front();
	return true;
}
template<typename T>
T WorkQueue<T>::pop_wait(int waitMS)
{
	std::unique_lock<std::mutex> lck(*this);
	wait_ready(lck,waitMS);
	if (_items.empty())
		return false;
	T ref = std::move(_items.front());
	_items.pop_front();
	return ref;
}
typedef std::function<void()> Task;

class ThreadPool : public noncopyable 
{
	public:
		ThreadPool(int threads, int taskCapacity = 0, bool start=true);
		~ThreadPool();
		void start();
		ThreadPool& exit() {_tasks.exit(); return *this;};
		void join();

		bool addTask(Task&& task);
	//	bool addTask(Task& task); ?????
		size_t taskSize() {return _tasks.size();}
	private:
		WorkQueue<Task> _tasks;
		std::vector<std::thread> _threads;
};

#endif
