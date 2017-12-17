#include <iostream>
#include "Epoller.h"
#include "Event_base.h"
#include <string.h>

PollerEpoll* createPoller()
{
	return new PollerEpoll();
}

PollerEpoll::PollerEpoll()
{
	_fd = epoll_create(EPOLL_CLOEXEC);
	std::cout<<"PollerEpoll::PollerEpoll()"<<std::endl;
}

PollerEpoll::~PollerEpoll()
{
	std::cout<<"destroying poller: "<<_fd<<std::endl;
	while (_liveChannels.size())
	{

	}
}

void PollerEpoll::addChannel(Channel* ch)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = ch->events();
	ev.data.ptr = ch;
	printf("adding channel id[%lld] fd[%d] events[%d] epoll[%d]",(long long)ch->id(),ch->fd(),ev.events,_fd);
	epoll_ctl(_fd,EPOLL_CTL_ADD,ch->fd(),&ev);
	_liveChannels.insert(ch);
}

void PollerEpoll::updateChannel(Channel* ch)
{
	struct epoll_event ev;
	memset(&ev,0,sizeof(ev));
	ev.events = ch->events();
	ev.data.ptr = ch;
	printf("modifying channel id[%lld] fd[%d] events read:[%d] write:[%d] epoll:[%d]",
			(long long)ch->id(), ch->fd(),ev.events & EPOLLIN, ev.events & EPOLLOUT,_fd);
	epoll_ctl(_fd,EPOLL_CTL_MOD,ch->fd(),&ev);
}

void PollerEpoll::removeChannel(Channel* ch)
{
	printf("deletng channel id[%lld] fd[%d] epoll[%d]",
		(long long)ch->id(),ch->fd(),_fd);
	_liveChannels.erase(ch);
	for (int i = _lastActive; i>= 0;i--)
	{
		if (ch == _activeEvs[i].data.ptr)
		{
			_activeEvs[i].data.ptr = NULL;
			break;
		}
	}
}

void PollerEpoll::loop_once(int waitMs)
{
	int64_t ticks = util::timeMilli();
	_lastActive = ::epoll_wait(_fd,_activeEvs,kMaxEvents,waitMs);
	int64_t used = util::timeMilli() - ticks;
	printf("epoll waitMs[%d] used [%lld] millsecond",
		waitMs, (long long)used);
	while (--_lastActive >= 0)
	{
		int i = _lastActive;
		Channel* ch = (Channel*)_activeEvs[i].data.ptr;
		int events = _activeEvs[i].events;
		if (ch)
		{
			if (events & kReadEvent)
			{
				printf("channel id[%lld] fd[%d] handle read",
						(long long)ch->id(),ch->fd());
				ch->handleRead();
			}
			else if (events & kWriteEvent)
			{
				printf("channel id[%lld] fd[%d] handle read",
						(long long)ch->id(),ch->fd());
				ch->handleWrite();
			}
			else{
				printf("unexpected poller events\n");
			}
		}
	}
}
