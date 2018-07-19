/*********************************************
 * politburo.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 30 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "politburo.h"
#include "proactor.h"
#include "postman.h"
#include "rawsock_postman.h"
#include "pcap_postman.h"
#include "tcp_postman.h"
#include "realtime_capturer.h"
#include "postoffice.h"
#include "cute_logger.h"

politburo g_politburo;

politburo::politburo()
{
}

politburo::~politburo()
{
}

int politburo::read_from_pcapfile(const std::string& path, const std::string& filter)
{
	return session_manager::read_from_pcapfile(path, filter);
}

boost::shared_ptr<boost::asio::deadline_timer> politburo::enqueue_a_timer_handler(int asio_idx,
							boost::posix_time::time_duration td, WaitHandler handler)
{
	boost::shared_ptr<boost::asio::deadline_timer> timer = g_proactor.produce_a_timer(td);
	timer->async_wait(_strands[asio_idx]->wrap(handler));

	return timer;
}

void politburo::get_ready()
{
	int i;
	_asio_thrd_num = g_configuration.get_asio_thrd_num();
	_strands.resize(_asio_thrd_num);

	boost::shared_ptr<postman> m;
	switch (g_configuration.get_sniff_method())
	{
	case configuration::SNIFF_RAW:
		m.reset(new rawsock_postman());
		break;

	case configuration::SNIFF_PCAP:
		m.reset(new pcap_postman());
		break;

	case configuration::SNIFF_TCP:
		m.reset(new tcp_postman());
		break;
	}

	m->get_ready();

	for (i = 0; i < _asio_thrd_num; i++)
	{
		session_manager::instance(i).get_ready();
		postoffice::instance(i).get_ready(m);
	}

	for (i = 0; i < _asio_thrd_num; i++)
	{
		_strands[i].reset(new boost::asio::strand(g_proactor.get_io_service()));
		_strands[i]->post(boost::bind(&politburo::post_the_jobs, this, i));
	}
}

void politburo::post_the_jobs(int asio_idx)
{
	g_realtime_capturer.pluck_out_and_inject_realtime_ippkts(asio_idx, 100);

	try
	{
		postoffice::instance(asio_idx).send_packets_to_wire();
		postoffice::instance(asio_idx).recv_packets_from_wire();
	}
	catch(std::exception& e)
	{
		g_logger.printf("%s", boost::diagnostic_information(e).c_str());
	}

	_strands[asio_idx]->post(boost::bind(&politburo::post_the_jobs, this, asio_idx));
}
