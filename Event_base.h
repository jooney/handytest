#ifndef __EVENT_BASE_H__
#define __EVENT_BASE_H__

#include "threads.h"
#include "Util.h"
#include "Epoller.h"

class EventBases : public noncopyable
{
	//	virtual EventBase* allocBase() = 0;	
};

class EventsImp;
class EventBase : public EventBases 
{
	public:
		EventBase(int taskcapacity=0);
		~EventBase();
		void loop();
		void loop_once(int waitMs);
		bool cancel(TimerId);
		TimerId runAt(int64_t milli, const Task& task, int64_t interval = 0);
		TimerId runAt(int64_t milli, Task&& task, int64_t interval =0);
	//	TimerId runAfter(int64_t milli, const Task& task,int64_t interval = 0)
	//	{return runAt()}
	//	TimerId runAfter(int64_t milli, Task&& task, int64_t interval=0);

		EventBase& exit();
		bool exited();
		void wakeup();
		void safeCall(Task&& task);	
		virtual EventBase* allocBase(){return this;}
	public:
		std::unique_ptr<EventsImp> _imp;
};

class MultiBase : public EventBases
{
	private:
		std::atomic<int> _id;
		std::vector<EventBase> _base;
	public:
		MultiBase(int sz)
			:_id(0),_base(sz){}
		void loop();
		virtual EventBase* allocBase()
		{
			int c =_id++;
			return &_base[c%_base.size()];
		}
		MultiBase& exit()
		{
			for (auto& b : _base)
			{
				b.exit();
			}
			return *this;
		}
};

class Channel : public noncopyable 
{
	private:
		EventBase* _base;
		PollerEpoll* _poller;
		int       _fd;
		short     _events;
		int64_t   _id;
		std::function<void()> _readcb, _writecb, _errorcb;
	public:
		Channel(EventBase* base, int fd, int events);
		~Channel();
		EventBase* getBase() {return _base;}
		int fd() {return _id;}
		//channel's id
		int64_t   id() {return _id;}
		short events(){return _events;}
		//close channel
		void close();
		//attach callback to channel 
		void onRead(const Task& readcb){ _readcb = readcb;}
		void onWrite(const Task& writecb){_writecb = writecb;}
		void onRead(Task&& readcb){_readcb = std::move(readcb);}
		void onWrite(Task&& writecb){_writecb = std::move(writecb);}
		// ready to read and write 
		void enableRead(bool enable);
		void enableWrite(bool enable);
		void enableReadWrite(bool readable, bool writeable);
		bool readEnabled();
		// handle read write event  
		void handleRead() {_readcb();}
		void handleWrite(){_writecb();}
};





#endif
