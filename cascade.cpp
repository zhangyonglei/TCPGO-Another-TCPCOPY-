/*********************************************
 * cascade.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 19 Mar, 2014
 ********************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include "cascade.h"
#include "misc.h"

#define FIFO_PATH "/tmp/horos.fifo"

cascade g_cascade;

cascade::cascade()
{
}

cascade::~cascade()
{
	stop();
}

void cascade::stop()
{
	_done = true;
	_forwarder.join();
	if (_fifo_fd > 0)
	{
		close(_fifo_fd);
		_fifo_fd = -1;
	}
}

void cascade::restart()
{
	stop();
	_signals->async_wait(boost::bind(&cascade::restart, this));
	_done = false;
	ready_go();
}

void cascade::enable_sigpipe()
{
	int ret;
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGUSR2);
    ret = pthread_sigmask(SIG_UNBLOCK, &set, NULL);
    assert(ret == 0);
}

void cascade::ready_go()
{
	enable_sigpipe();

	int ret = mkfifo(FIFO_PATH, 0666);
	if (0 != ret && EEXIST != errno)
	{
		perror("mkfifo");
		abort();
	}

	_io_service.reset(new boost::asio::io_service);
	_signals.reset(new boost::asio::signal_set(*_io_service, SIGPIPE, SIGUSR2));
	_signals->async_wait(boost::bind(&cascade::restart, this));

	_count = 0;
	_done = false;
	_fifo_fd = -1;

	_forwarder = boost::thread(boost::bind(&cascade::run, this));
}

void cascade::push_back(boost::shared_ptr<MemBlock> block)
{
	bool success = _mem_blocks.push(block);
	if (success)
	{
		_count++;
		if (_count == 1)
		{
			boost::mutex::scoped_lock lock(_mutex);
			_con_var.notify_one();
		}
	}
}

void cascade::run()
{
	bool success;
	boost::shared_ptr<MemBlock> mem_block;

again:
	_fifo_fd = open(FIFO_PATH, O_WRONLY);
	if (_fifo_fd < 0)
	{
		perror("open");
		abort();
	}

	while (!_done)
	{
		success = _mem_blocks.pop(mem_block);
		if (success)
		{
			MemBlock& m = *mem_block;
			assert(_count > 0);
			_count--;
			int ret = write(_fifo_fd, m.data(), m.size());
			if (ret < 0)
			{
				goto again;
			}
		}
		else  // no jobs to be done at present.
		{
			boost::mutex::scoped_lock lock(_mutex);
			_con_var.timed_wait(lock, boost::posix_time::milliseconds(1000));
			//std::cout << "quit timed_wait" << std::endl;
		}
	}
}
