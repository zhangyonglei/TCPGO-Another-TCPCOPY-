/*********************************************
 * testsuite.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Feb, 2014
 ********************************************/

#include "testsuite.h"

testsuite g_testsuite;

testsuite::testsuite()
{
}

testsuite::~testsuite()
{
	_tester->join();
}

void testsuite::get_ready()
{
	_tester.reset(
			new boost::thread(boost::bind(&testsuite::run_worker, this))
	);
}

void testsuite::report_sess_traffic(std::string client_src_ip,
						 uint16_t port,
						 const std::list<ip_pkt>& traffic,
						 tcpsession::cause_of_death cause)
{

}

void testsuite::run_worker()
{

}
