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

protected:
	virtual ~evt_workhorse(){}
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

	/**
	 * Why this method is called big bang?
	 * GOOD QUESTION. That's 'cos:
	 * Our whole universe was in a hot dense state,
	 * Then nearly fourteen billion years ago expansion started. Wait...
     * The Earth began to cool.
	 * The autotrophs began to drool.
	 * Neanderthals developed tools.
	 * We built a wall. (We built the pyramids.)
	 * Math, science, history, unraveling the mystery,
	 * That all started with a big bang!
	 */
	void bigbang();

private:
	int _epoll_fd;
	std::map<int, evt_workhorse*> _workhorses;
};

#endif /* _POLLER_H_ */