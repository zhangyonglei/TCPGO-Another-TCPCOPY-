/*********************************************
 * poller.h
 * Author: kamuszhou@tencent.com,16236914@qq.com
 * website: www.dogeye.net
 * Created on: 13 Dec, 2013
 ********************************************/

#ifndef _POLLER_H_
#define _POLLER_H_

#include <map>

class poller;
extern poller g_poller;

class evt_workhorse
{
public:
	virtual void pollin_handler(int fd) = 0;
	virtual void pollout_handler(int fd) = 0;
};

/**
 * poller is just c++ wrapper of epoll.
 */
class poller
{
public:
	enum poll_type{POLLIN, POLLOUT};

	poller();
	virtual ~poller();

	void register_evt(int fd, poll_type type, evt_workhorse* workhorse);
	void deregister_evt(int fd);
	void run();

private:
	int _epoll_fd;
	std::map<int, evt_workhorse*> _workhorses;
};

#endif /* _POLLER_H_ */
