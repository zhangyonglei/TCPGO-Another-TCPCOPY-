/*********************************************
 * reactor.cpp
 * Author: kamuszhou@tencent.com,kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: 13 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include <sys/epoll.h>
#include "misc.h"
#include "reactor.h"
#include "thetimer.h"
#include "postoffice.h"

#define MAX_EVENTS_COUNT 32

reactor g_reactor;

reactor::reactor()
{
	_stop = false;
	_epoll_fd = epoll_create(1);
	if (_epoll_fd < 0)
	{
		perror("epoll_create");
		abort();
	}
}

reactor::~reactor()
{
	close(_epoll_fd);
}

void reactor::register_evt(int fd, poll_type type, evt_workhorse* workhorse)
{
	struct epoll_event evt;

	if (_workhorses.size() == MAX_EVENTS_COUNT)
	{
		std::cerr <<"Increase the macro MAX_EVENTS_COUNT in file "__FILE__;
		abort();
	}

	memset(&evt, 0, sizeof(evt));
	if (type == MYPOLLIN)
	{
		evt.events |= EPOLLIN;
	}
	if (type == MYPOLLOUT)
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

void reactor::deregister_evt(int fd)
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

void reactor::bigbang()
{
	int nfds, n;
	int fd;
	evt_workhorse *workhorse;
	struct epoll_event  events[MAX_EVENTS_COUNT];

	//sigset_t sigmask;
	//sigemptyset(&sigmask);
	//sigaddset(&sigmask, SIGALRM);

	while(true)
	{
		// Kernel Version is too old that doesn't support epoll_wait.
		//nfds = epoll_pwait(_epoll_fd, events, MAX_EVENTS_COUNT, 10/*in unit of millisecond*/, &sigmask);
		nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS_COUNT, 10/*in unit of millisecond*/);
		if (_stop)
			return;

		if (nfds == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}

			perror("epoll_pwait");
			abort();
		}

		for (n = 0; n < nfds; ++n)
		{
			fd = events[n].data.fd;
			workhorse = _workhorses.at(fd);
			if (events[n].events & EPOLLIN)
			{
				workhorse->pollin_handler(fd);
			}
			if (events[n].events & EPOLLOUT)
			{
				workhorse->pollout_handler(fd);
			}
		}

		g_timer.loop_through_all_timer_event();

		g_postoffice.recv_packets_from_wire();
		g_postoffice.send_packets_to_wire();
	}
}

void reactor::stop()
{
	_stop = true;
}
