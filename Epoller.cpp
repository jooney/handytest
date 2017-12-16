#include "Epoller.h"
#include "Event_base.h"
#include <iostream>

PollerEpoll::PollerEpoll()
{
	_fd = epoll_create(EPOLL_CLOEXEC);
	std::cout<<"PollerEpoll::PollerEpoll()"<<std::endl;
}

PollerEpoll::~PollerEpoll()
{
}

PollerEpoll* createPoller()
{
	return new PollerEpoll();
}
