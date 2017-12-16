#ifndef __EPOLLER_H__
#define __EPOLLER_H__ 
#include "Util.h"
#include <sys/epoll.h>

const int kMaxEvents = 2000;
const int kReadEvent = EPOLLIN;
const int kWriteEvent = EPOLLOUT;

class PollerEpoll
{
	private:
		int64_t _id;
		int _fd;
		int _lastActive;
		struct epoll_event _activeEvs[kMaxEvents];
	public:
		PollerEpoll();
		~PollerEpoll();
};

PollerEpoll* createPoller();


#endif
