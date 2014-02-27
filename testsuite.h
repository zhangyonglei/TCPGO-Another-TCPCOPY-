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
#include <boost/atomic.hpp>

class testsuite;
extern testsuite g_testsuite;

class testsuite
{
private:
	struct job_block
	{
		job_block(const std::string client_str_ip,
				  uint16_t port,
				  const std::list<ip_pkt>& traffic,
				  tcpsession::cause_of_death cause)
			:_traffic(traffic), _port(port), _client_str_ip(client_str_ip), _cause(cause)
		{
		}

		std::string _client_str_ip;
		uint16_t  _port;
		std::list<ip_pkt> _traffic;
		tcpsession::cause_of_death _cause;
	};

public:
	testsuite();
	virtual ~testsuite();

	void get_ready();

	/**
	 * @param port in host byte order
	 */
	void report_sess_traffic(const std::string& client_src_ip,
							 uint16_t port,
							 const std::list<ip_pkt>& traffic,
							 tcpsession::cause_of_death cause);

	void run_worker();

private:
	void load_lua_scripts();
	void load_shared_objects();

private:
	/**
	 * split the traffic to inbound traffic and outbound traffic.
	 */
	void split_traffic(const std::list<ip_pkt>& traffic, std::vector<char>& request, std::vector<char>& response);

	/**
	 * call all the test cases.
	 */
	void do_tests(const std::string& client_str_ip, uint16_t port,
				  const std::vector<char>& request, const std::vector<char>& response);

	/**
	 * save the traffic to the pcap file.
	 */
	void save_traffic(const std::list<ip_pkt>& traffic, const std::string& pcap_file);

private:
	typedef boost::lockfree::spsc_queue<boost::shared_ptr<job_block>, boost::lockfree::capacity<1024> > LockFreeQueue;
	LockFreeQueue _jobs;
	boost::shared_ptr<boost::thread> _tester;

	boost::atomic_int _count_jobs;
	boost::atomic<bool> _done;
	boost::mutex _mutex;
	boost::condition_variable  _con_var;

	pcap_t* _pcap_handle;  ///< used to create pcap files.
};

#endif /* _TESTSUITE_H_ */
