/*********************************************
 * testsuite.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Feb, 2014
 ********************************************/

#ifndef _TESTSUITE_H_
#define _TESTSUITE_H_

#include "misc.h"
#include "tcpsession.h"
#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>

class testsuite;
extern testsuite g_testsuite;

class testsuite
{
private:
	struct job_block
	{
		job_block(const std::list<ip_pkt>& traffic,
				  const std::string client_str_ip,
				  uint16_t port,
				  tcpsession::cause_of_death cause)
			:_traffic(traffic), _port(port), _client_str_ip(client_str_ip), _cause(cause)
		{
		}

		std::list<ip_pkt> _traffic;
		uint16_t  _port;
		std::string _client_str_ip;
		tcpsession::cause_of_death _cause;
	};

public:
	testsuite();
	virtual ~testsuite();

	void get_ready();

	/**
	 * @param port in host byte order
	 */
	void report_sess_traffic(std::string client_src_ip,
							 uint16_t port,
							 const std::list<ip_pkt>& traffic,
							 tcpsession::cause_of_death cause);

	void run_worker();

private:
	typedef boost::lockfree::spsc_queue<job_block, boost::lockfree::capacity<1024> > LockFreeQueue;
	LockFreeQueue _jobs;
	boost::shared_ptr<boost::thread> _tester;
};

#endif /* _TESTSUITE_H_ */
