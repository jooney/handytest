#include <iostream>
#include "Epoller.h"
#include "Event_base.h"
#include <string.h>
#include "log.h"
#include <unistd.h>

PollerEpoll* createPoller()
{
	return new PollerEpoll();
}

PollerEpoll::PollerEpoll()
{
	_fd = epoll_create(EPOLL_CLOEXEC);
	info("PollerEpoll::PollerEpoll() epoll's fd[%d]",_fd);
}

PollerEpoll::~PollerEpoll()
{
	info("destroying poller: epoll's fd[%d]",_fd);
	while (_liveChannels.size())
	{
		auto it = _liveChannels.begin();
		(*it)->close();	
	}
	::close(_fd);
	info("epoll's fd[%d] destroyed",_fd);
}

void PollerEpoll::addChannel(Channel* ch)
{
	struct epoll_event ev;
	memset(&ev, 0, sizeof(ev));
	ev.events = ch->events();
	ev.data.ptr = ch;
	info("epoll[%p] addChannel id[%lld] fd[%d] events[%d]",this,(long long)ch->id(),ch->fd(),ev.events);
	epoll_ctl(_fd,EPOLL_CTL_ADD,ch->fd(),&ev);
	_liveChannels.insert(ch);
}

void PollerEpoll::updateChannel(Channel* ch)
{
	struct epoll_event ev;
	memset(&ev,0,sizeof(ev));
	ev.events = ch->events();
	ev.data.ptr = ch;
	info("epoll[%p] updateChannel id[%lld] fd[%d] events read:[%d] write:[%d]",
			this,(long long)ch->id(), ch->fd(),ev.events & EPOLLIN, ev.events & EPOLLOUT);
	epoll_ctl(_fd,EPOLL_CTL_MOD,ch->fd(),&ev);
}

void PollerEpoll::removeChannel(Channel* ch)
{
	info("epoll[%p] removeChannel id[%lld] fd[%d]",
		this,(long long)ch->id(),ch->fd());
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
	info("epoll[%p] PollerEpoll::loop_once waitMs[%d]",this,waitMs);
	int64_t ticks = util::timeMilli();
	_lastActive = ::epoll_wait(_fd,_activeEvs,kMaxEvents,waitMs);
	int64_t used = util::timeMilli() - ticks;
	info("epoll[%p] waitMs[%d] used [%lld] millsecond",this,waitMs, (long long)used);
	while (--_lastActive >= 0)
	{
		int i = _lastActive;
		Channel* ch = (Channel*)_activeEvs[i].data.ptr;
		int events = _activeEvs[i].events;
		if (ch)
		{
			if (events & kReadEvent)
			{
				info("epoll[%p] channel id[%lld] fd[%d] handle read",this,(long long)ch->id(),ch->fd());
				ch->handleRead();
			}
			else if (events & kWriteEvent)
			{
				info("epoll[%p] channel id[%lld] fd[%d] handle write",this,(long long)ch->id(),ch->fd());
				ch->handleWrite();
			}
			else{
				info("epoll[%p] unexpected poller events",this);
			}
		}
	}
}
