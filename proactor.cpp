/*********************************************
 * proactor.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 28 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "proactor.h"

proactor g_proactor;

proactor::proactor() : _work(_io_service)
{
}

proactor::~proactor()
{
	_io_service.stop();
	_threads.join_all();
}

void proactor::get_ready()
{
	for (int i = 0; i < _threads_count; i++)
	{
		_threads.create_thread(boost::bind(&boost::asio::io_service::run, &_io_service));
	}
}


boost::shared_ptr<boost::asio::deadline_timer> proactor::produce_a_timer(boost::posix_time::time_duration td)
{
	return boost::make_shared<boost::asio::deadline_timer>(boost::ref(_io_service), boost::ref(td));
}
