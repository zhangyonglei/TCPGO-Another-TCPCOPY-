/*********************************************
 * poller.cpp
 * Author: kamuszhou@tencent.com,16236914@qq.com
 * website: www.dogeye.net
 * Created on: 13 Dec, 2013
 ********************************************/

#include "misc.h"
#include "poller.h"
#include <sys/epoll.h>

#define MAX_EVENTS_COUNT 8

poller g_poller;

poller::poller()
{
	_stop = false;
	_epoll_fd = epoll_create(1);
	if (_epoll_fd < 0)
	{
		perror("epoll_create");
		abort();
	}
}

poller::~poller()
{
	close(_epoll_fd);
}

void poller::register_evt(int fd, poll_type type, evt_workhorse* workhorse)
{
	struct epoll_event evt;

	if (_workhorses.size() == MAX_EVENTS_COUNT)
	{
		std::cerr <<"Increase the macro MAX_EVENTS_COUNT in file "__FILE__;
		abort();
	}

	memset(&evt, 0, sizeof(evt));
	if (type == POLLIN)
	{
		evt.events |= EPOLLIN;
	}
	if (type == POLLOUT)
	{
		evt.events |= EPOLLOUT;
	}

	evt.data.fd = fd;
	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &evt) == -1)
	{
		perror("epoll_ctl: ");
		abort();
	}

	_workhorses[fd] = workhorse;
}

void poller::deregister_evt(int fd)
{
	int ret;
	ret = epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
	if (ret < 0)
	{
		perror("epoll_ctl: ");
		abort();
	}

	ret = _workhorses.erase(fd);
	assert(1 == ret);
}

void poller::bigbang()
{
	int nfds, n;
	int fd;
	evt_workhorse *workhorse;
	struct epoll_event  events[MAX_EVENTS_COUNT];

	while(true)
	{
		nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS_COUNT, 100);
		if (_stop)
			return;

		if (nfds == -1 && errno == EINTR)
		{
			perror("epoll_pwait");
			abort();
		}

		for (n = 0; n < nfds; ++n)
		{
			fd = events[n].data.fd;
			workhorse = _workhorses.at(fd);
			if (events[n].events | EPOLLIN)
			{
				workhorse->pollin_handler(fd);
			}
			if (events[n].events | EPOLLOUT)
			{
				workhorse->pollout_handler(fd);
			}
		}
	}
}

void poller::stop()
{
	_stop = true;
}
