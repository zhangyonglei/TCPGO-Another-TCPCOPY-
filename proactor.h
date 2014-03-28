/*********************************************
 * proactor.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 28 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _PROACTOR_H_
#define _PROACTOR_H_

#include "misc.h"
#include <boost/asio.hpp>

class proactor;
extern proactor g_proactor;

class proactor
{
public:
	proactor();

	void get_ready();

	virtual ~proactor();

	boost::shared_ptr<boost::asio::deadline_timer>  produce_a_timer(boost::posix_time::time_duration td);

private:
	boost::asio::io_service _io_service;
	boost::asio::io_service::work _work;

	static const int _threads_count = 2;
	boost::thread_group _threads;
};

#endif /* _PROACTOR_H_ */
