/*********************************************
 * cascade.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 19 Mar, 2014
 ********************************************/

#ifndef _CASCADE_H_
#define _CASCADE_H_

#include "misc.h"
#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>
#include <boost/asio/signal_set.hpp>

class cascade;
extern cascade g_cascade;

/**
 * To chain together many horos instances running on different machines.
 */
class cascade
{
public:
	typedef std::vector<char> MemBlock;
	typedef boost::lockfree::spsc_queue<boost::shared_ptr<MemBlock>, boost::lockfree::capacity<10240> > LockFreeQueue;

public:
	cascade();
	virtual ~cascade();

	void ready_go();

	void restart();

	void push_back(boost::shared_ptr<MemBlock> block);

	void run();

	void stop();

private:
	void enable_sigpipe();

private:
	LockFreeQueue _mem_blocks;

	boost::thread _forwarder;
	boost::atomic_int _count;
	boost::atomic<bool> _done;
	boost::mutex _mutex;
	boost::condition_variable  _con_var;

	boost::scoped_ptr<boost::asio::io_service> _io_service;
	boost::scoped_ptr<boost::asio::signal_set> _signals;

	int _fifo_fd;
};

#endif /* _CASCADE_H_ */