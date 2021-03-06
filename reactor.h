/*********************************************
 * reactor.h
 * Author: kamuszhou@tencent.com,kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: 13 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _REACTOR_H_
#define _REACTOR_H_

#include <map>

class reactor;
extern reactor g_reactor;

class evt_workhorse
{
public:
	virtual void pollin_handler(int fd) = 0;
	virtual void pollout_handler(int fd) = 0;

protected:
	virtual ~evt_workhorse(){}
};

/**
 * reactor design pattern.
 * reactor is just a c++ wrapper of epoll.
 */
class reactor
{
public:
	enum poll_type{MYPOLLIN, MYPOLLOUT};

	reactor();
	virtual ~reactor();

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

	void stop();

private:
	volatile bool _stop;
	int _epoll_fd;
	std::map<int, evt_workhorse*> _workhorses;
};

#endif /* _REACTOR_H_ */
