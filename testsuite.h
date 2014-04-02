/*********************************************
 * testsuite.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Feb, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _TESTSUITE_H_
#define _TESTSUITE_H_

#include <boost/thread/thread.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/atomic.hpp>
#include "misc.h"
#include "tcpsession.h"

class testsuite;
extern testsuite g_testsuite;

class testsuite
{
friend class mylua;

private:
	struct job_block
	{
		job_block(const std::string client_str_ip,
				  uint16_t port,
				  const std::list<boost::shared_ptr<ip_pkt> >& traffic,
				  tcpsession::cause_of_death cause)
			: _client_str_ip(client_str_ip), _port(port), _traffic(traffic), _cause(cause)
		{
		}

		std::string _client_str_ip;
		uint16_t  _port;
		std::list<boost::shared_ptr<ip_pkt> > _traffic;
		tcpsession::cause_of_death _cause;
	};

public:
	testsuite();
	virtual ~testsuite();

	void ready_go();

	/**
	 * stop the testing process. The caller may need to invoke ready_go()
	 * later to restart testing processing.
	 */
	void stop();

	/**
	 * @param port in host byte order
	 */
	void report_sess_traffic(int asio_idx, const std::string& client_src_ip,
							 uint16_t port,
							 const std::list<boost::shared_ptr<ip_pkt> >& traffic,
							 tcpsession::cause_of_death cause);

	void run_worker();

private:
	void load_lua_scripts();
	void load_shared_objects();

private:
	/**
	 * split the traffic to inbound traffic and outbound traffic.
	 * @return 0 on success. If the traffic lost packet, or other abnormal conditions occurred, non-zero will returned.
	 */
	int split_traffic(const std::list<boost::shared_ptr<ip_pkt> >& traffic,
			std::vector<char>& request, std::vector<char>& response);

	/**
	 * call all the test cases.
	 */
	void do_tests(const std::string& client_str_ip, uint16_t port,
				  const std::vector<char>& request, const std::vector<char>& response);

	/**
	 * save the traffic to the pcap file.
	 * @param force If this parameter is set to true, then the file number ceiling control will be suppressed.
	 */
	static int save_traffic(const std::list<boost::shared_ptr<ip_pkt> >& traffic, const std::string& pcap_file, bool force);

private:
	int save_traffic(const std::string& pcap_file);

private:
	typedef boost::lockfree::spsc_queue<boost::shared_ptr<job_block>, boost::lockfree::capacity<5000> > LockFreeQueue;
	std::vector<boost::shared_ptr<LockFreeQueue> > _queue_jobs;
	std::vector<boost::shared_ptr<boost::atomic_int> > _queue_counts;
	int _asio_thrd_num;

	boost::thread _tester;

	boost::atomic<bool> _done;
	boost::mutex _mutex;
	boost::condition_variable  _con_var;

	// // Don't kown why pcap_open_dead doesn't work. So i switched to write a pcap file manually.
//	pcap_t* _pcap_handle;  ///< used to create pcap files.
	std::list<boost::shared_ptr<ip_pkt> >* _current_traffic_on_test; // un-graceful.
};

#endif /* _TESTSUITE_H_ */
