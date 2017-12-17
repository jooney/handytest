#include "Event_base.h"
#include <fcntl.h>
#include <map>
#include <unistd.h>
#include "net.h"

struct TimerRepeatable
{
	int64_t  at;   //current timer timeout timestamp
	int64_t  interval;
	TimerId timerid;
	Task  cb;
};
class EventsImp
{
	public:
		EventBase*    _base;
		PollerEpoll*  _poller;	
		std::atomic<bool>  _exit;
//		int _wakeupFds[2];
		WorkQueue<Task> _tasks;
		std::map<TimerId, TimerRepeatable> _timerReps;
		std::map<TimerId, Task> _timers;
		std::atomic<int64_t> _timerSeq;
		//  IdleConntctions
		bool idleEnabled;
	public:
		EventsImp(EventBase* base, int taskCap);
		~EventsImp();
		void init();
		void callIdles();
		void handleTimeouts();
		//void refreshNearest(const TimerId& tid);
		// eventbase  functions 
		EventBase& exit() {_exit = true; wakeup(); return *_base;}
		bool exited() {return _exit;}
		void safeCall(Task&& task) {_tasks.push(std::move(task));wakeup();}
		void loop();
		void loop_once(int waitMs) {}
		void wakeup()
		{
		
		}
		bool cancel(TimerId timerid);
		TimerId runAt(int64_t milli, Task&& task, int64_t interval);

};

EventBase::EventBase(int capacity)
{
	_imp.reset(new EventsImp(this,capacity));
	_imp->init();
}
EventBase::~EventBase(){}

EventBase& EventBase::exit() {return _imp->exit();}

bool EventBase::exited() {return _imp->exited();}

void EventBase::safeCall(Task&& task){_imp->safeCall(std::move(task));}

void EventBase::wakeup(){_imp->wakeup();}

void EventBase::loop(){_imp->loop();}

void EventBase::loop_once(int waitMs){_imp->loop_once(waitMs);}

bool EventBase::cancel(TimerId timerid) {return _imp->cancel(timerid);}

EventsImp::EventsImp(EventBase* base, int taskCap)
	:_base(base),_poller(createPoller()),_exit(false),_tasks(taskCap),
	_timerSeq(0),idleEnabled(false)
{
}

void EventsImp::loop()
{
	while (!_exit)
	{
		loop_once(10000);
	}
	_timerReps.clear();
	_timers.clear();
	loop_once(0); //?????
}

void EventsImp::init()
{
//	int r = pipe(_wakeupFds);
//	r = util::addFdFlag(_wakeupFds[0],FD_CLOEXEC);
//	r = util::addFdFlag(_wakeupFds[1],FD_CLOEXEC);
//	std::cout<<"wakeup pipe created: "<<_wakeupFds[0]<<":"<<_wakeupFds[1]<<std::endl;
//	Channel * ch = new Channel(_base,_wakeupFds[0],kReadEvent);

}

void EventsImp::handleTimeouts()
{
	//TODO
}
EventsImp::~EventsImp()
{
	delete _poller;
}

bool EventsImp::cancel(TimerId timerid)
{
	return false;
}

void MultiBase::loop()
{
	int sz = _base.size();
	std::vector<std::thread> ths(sz -1);
	for (int i = 0;i<sz - 1;i++)
	{
		std::thread t([this,i]{_base[i].loop();});
		ths[i].swap(t);
	}
	_base.back().loop();
	for (int i = 0;i<sz -1;i++)
		ths[i].join();
}
Channel::Channel(EventBase* base,int fd,int events)
	:_base(base),_fd(fd),_events(events)
{
	net::setNonBlock(_fd);
	static std::atomic<int64_t> id(0);
	_id = ++id;
	_poller = _base->_imp->_poller;
	_poller->addChannel(this);
}

Channel::~Channel()
{
	close();
}

void Channel::close()
{
	if (_fd >= 0)
	{
		printf("close channel id[%lld] fd[%d]",(long long)_id, _fd);
		_poller->removeChannel(this);
		::close(_fd);
		_fd = -1;
		handleRead();
	}
}
