/*********************************************
 * testsuite.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  http://blog.ykyi.net
 * Created on: 24 Feb, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "testsuite.h"
#include "cute_logger.h"
#include "mylua.h"
#include <boost/format.hpp>

struct pcap_hdr_t {
        uint32_t magic_number;   /* magic number */
        uint16_t version_major;  /* major version number */
        uint16_t version_minor;  /* minor version number */
        int32_t  thiszone;       /* GMT to local correction */
        uint32_t sigfigs;        /* accuracy of timestamps */
        uint32_t snaplen;        /* max length of captured packets, in octets */
        uint32_t network;        /* data link type */
}; // 24 bytes

struct pcaprec_hdr_t {
        uint32_t ts_sec;         /* timestamp seconds */
        uint32_t ts_usec;        /* timestamp microseconds */
        uint32_t incl_len;       /* number of octets of packet saved in file */
        uint32_t orig_len;       /* actual length of packet */
};

testsuite g_testsuite;

testsuite::testsuite()
{
}

testsuite::~testsuite()
{
	stop();

//	if (NULL != _pcap_handle)
//	{
//		pcap_close(_pcap_handle);
//	}
}

void testsuite::ready_go()
{
	_done = false;
	_current_traffic_on_test = NULL;

	_asio_thrd_num = g_configuration.get_asio_thrd_num();
	_queue_jobs.resize(_asio_thrd_num);
	_queue_counts.resize(_asio_thrd_num);
	for (int i = 0; i < _asio_thrd_num; i++)
	{
		_queue_jobs[i].reset(new LockFreeQueue());
		_queue_counts[i].reset(new boost::atomic_int);
		_queue_counts[i]->operator=(0);
	}

	if (!g_configuration.get_lua_scripts_home())
	{
		return;
	}

//	if (NULL == _pcap_handle)
//	{
//		_pcap_handle == pcap_open_dead(DLT_RAW, 65535 /* snaplen */);
//		assert(NULL != _pcap_handle);
//	}

	_tester = boost::thread(boost::bind(&testsuite::run_worker, this));
}

void testsuite::stop()
{
	_done = true;
	_tester.join();
}

void testsuite::report_sess_traffic(int asio_idx,
		                 const std::string& client_src_ip,
						 uint16_t port,
						 const std::list<boost::shared_ptr<ip_pkt> >& traffic,
						 tcpsession::cause_of_death cause)
{
	boost::shared_ptr<job_block> job(new job_block(client_src_ip, port, traffic, cause));
	bool success = _queue_jobs[asio_idx]->push(job);
	if (success)
	{
		_queue_counts[asio_idx]->operator++();
		// here is a time window that the calling thread will be switched out.
		if (*_queue_counts[asio_idx] <= 1)
		{
			boost::mutex::scoped_lock lock(_mutex);
			_con_var.notify_one();
		}
	}
}

void testsuite::run_worker()
{
	bool success;
	bool have_jobs_done;
	boost::shared_ptr<job_block> job;

	load_lua_scripts();
//	load_shared_objects();

	while (!_done)
	{
		have_jobs_done = false;
		for (int i = 0; i < _asio_thrd_num; i++)
		{
			success = _queue_jobs[i]->pop(job);
			if (success)
			{
				have_jobs_done = true;
				_queue_counts[i]->operator--();
				//std::cout << "consumed " << job->_client_str_ip << std::endl;
				//std::cout << _count_jobs << " left" << std::endl;

				_current_traffic_on_test = &job->_traffic;

				// let's do the job.
				// this session closes unexpectedly.
				if (job->_cause != tcpsession::PASSIVE_CLOSE && job->_cause != tcpsession::ACTIVE_CLOSE)
				{
					std::ostringstream ss;
					const char* cause = tcpsession::map_cause_code_to_str(job->_cause);
					ss << boost::format("%s_accident_death_%s_%d.pcap") %
							cause % job->_client_str_ip % job->_port;
					save_traffic(job->_traffic, ss.str(), false);

					_current_traffic_on_test = NULL;
					continue;
				}

				const std::list<boost::shared_ptr<ip_pkt> >& traffic = job->_traffic;
				std::vector<char> request;
				std::vector<char> response;
				int integrity = split_traffic(traffic, request, response);
				if (0 == integrity)  // if the traffic lost packet, do_tests won't be performed.
				{
					do_tests(job->_client_str_ip, job->_port, request, response);
				}

				_current_traffic_on_test = NULL;
			}
		} // end of for loop all the queues.

		if (!have_jobs_done && !_done)
		{
			boost::mutex::scoped_lock lock(_mutex);
			_con_var.timed_wait(lock, boost::posix_time::milliseconds(1000));
			//std::cout << "quit timed_wait" << std::endl;
		}
	}// end of while(!_done) loop
}

int testsuite::split_traffic(const std::list<boost::shared_ptr<ip_pkt> >& traffic,
		std::vector<char>& request, std::vector<char>& response)
{
	int retcode;
	int request_traffic_size, response_traffic_size;
	uint16_t server_port;

	uint32_t next_expected_seq_host, next_expected_seq_peer;

	int response_copied_bytes, request_copied_bytes;

	retcode = 0;
	request_traffic_size = 0;
	response_traffic_size = 0;

	server_port = g_configuration.get_dst_port();

	std::list<boost::shared_ptr<ip_pkt> > traffic_copy(traffic);
	traffic_copy.sort();
	traffic_copy.unique();

	for (std::list<boost::shared_ptr<ip_pkt> >::const_iterator ite = traffic_copy.begin();
		 ite != traffic_copy.end();
		 ++ite)
	{
		const ip_pkt* pkt = (*ite).get(); // for debug's convenience;
		bool is_syn_set = pkt->is_syn_set();
	//	bool is_fin_set = pkt->is_fin_set();
		int tcp_payload_len = pkt->get_tcp_payload_len();
		uint32_t seq = pkt->get_seq();


		if (pkt->get_src_port() == server_port )
		{
			if (!is_syn_set && 0 != tcp_payload_len)
			{
				if (next_expected_seq_host != seq)
				{
					retcode = -1;
					goto _exit;
				}
			}

			if (is_syn_set)
			{
				next_expected_seq_host = seq + 1;
			}
			else if (0 != tcp_payload_len)
			{
				next_expected_seq_host = seq + tcp_payload_len;
			}

			response_traffic_size += tcp_payload_len;
		}
		else
		{
			if (!is_syn_set && 0 != tcp_payload_len)
			{
				if (next_expected_seq_peer != seq)
				{
					retcode = -1;
					goto _exit;
				}
			}

			if (is_syn_set)
			{
				next_expected_seq_peer = seq + 1;
			}
			else
			{
				next_expected_seq_peer = seq + tcp_payload_len;
			}

			request_traffic_size += tcp_payload_len;
		}
	}

	request.resize(request_traffic_size);
	response.resize(response_traffic_size);

	response_copied_bytes = 0;
	request_copied_bytes = 0;
	for (std::list<boost::shared_ptr<ip_pkt> >::const_iterator ite = traffic_copy.begin();
		 ite != traffic_copy.end();
		 ++ite)
	{
		int payload_len = (*ite)->get_tcp_payload_len();
		if ((*ite)->get_src_port() == server_port )
		{
			memcpy(&response[0] + response_copied_bytes, (*ite)->get_tcp_payload(), payload_len);
			response_copied_bytes += payload_len;
		}
		else
		{
			memcpy(&request[0] + request_copied_bytes, (*ite)->get_tcp_payload(), payload_len);
			request_copied_bytes += payload_len;
		}
	}

_exit:
	return retcode;
}

int testsuite::save_traffic(const std::list<boost::shared_ptr<ip_pkt> >& traffic,
		const std::string& pcap_file, bool force)
{
//	pcap_dumper_t *pdumper;
//
//	pdumper = pcap_dump_open(_pcap_handle, pcap_file.c_str());
//
//	if (NULL == pdumper)
//	{
//        g_logger.printf("Error opening savefile \"%s\" for writing: %s\n",
//                 pcap_file.c_str(), pcap_geterr(_pcap_handle));
//		return;
//	}

	// Ugly code. just want to implement it fast so i use the static variable here.
	// refactor related logic to avoid use static variable later.
	static int saved_traffic_files_counter = 0;
	int retcode = 0;

	if (!force)
	{
		int limit = g_configuration.get_accidental_death_pcap_file_limit();
		if (saved_traffic_files_counter >= limit)
		{
			retcode = -2;
			return retcode;
		}

		saved_traffic_files_counter++;
	}

	struct pcap_hdr_t pcaphdr;
	struct pcaprec_hdr_t pkthdr;

	pcaphdr.magic_number = 0xa1b2c3d4;
//	pcaphdr.magic_number = 0xd4c3b2a1;
	pcaphdr.version_major = 2;
	pcaphdr.version_minor = 4;
	pcaphdr.thiszone = 0;
	pcaphdr.sigfigs = 0;
	pcaphdr.snaplen = 65535;
	pcaphdr.network = DLT_RAW;

	std::ofstream os(pcap_file.c_str(), std::ios::out | std::ios::binary);

	if (!os)
	{
		retcode = -1;
		g_logger.printf("Failed to open file %s.\n", pcap_file.c_str());
		return retcode;
	}

	os.write((const char*)&pcaphdr, sizeof(pcaphdr));

	for (std::list<boost::shared_ptr<ip_pkt> >::const_iterator ite = traffic.begin();
		 ite != traffic.end();
		 ++ite)
	{
		int pkt_tot_len = (*ite)->get_tot_len();
		memset(&pkthdr, 0, sizeof(pkthdr));
		// pkthdr.ts = 0;  // thie info is lost, i don't care. set it as zero.
		pkthdr.incl_len = pkt_tot_len;
		pkthdr.orig_len = pkt_tot_len;
		os.write((const char*)&pkthdr, sizeof(pkthdr));
		os.write((*ite)->get_starting_addr(), pkt_tot_len);

		//pcap_dump((u_char*)pdumper, &pkthdr, (const u_char*)ite->get_starting_addr());
	}

	os.close();

	return retcode;

//	pcap_dump_close(pdumper);
}

int testsuite::save_traffic(const std::string& pcap_file)
{
	int retcode;
	retcode = save_traffic(*_current_traffic_on_test, pcap_file, true);

	return retcode;
}

void testsuite::do_tests(const std::string& client_str_ip, uint16_t port,
					     const std::vector<char>& request, const std::vector<char>& response)
{
	g_mylua.run_lua_tests(client_str_ip, port, request, response);
}

void testsuite::load_lua_scripts()
{
	std::list<std::string> scripts;

	std::string home = *g_configuration.get_lua_scripts_home();
	find_files(home, ".*\\.lua$", scripts);

	for (std::list<std::string>::iterator ite = scripts.begin();
		ite != scripts.end();
		++ite)
	{
		g_mylua.load_lua_module(*ite);
	}
}

void testsuite::load_shared_objects()
{
	std::list<std::string> shared_objs;

	std::string home = *g_configuration.get_so_home();
	find_files(home, ".*\\.so$", shared_objs);

	for (std::list<std::string>::iterator ite = shared_objs.begin();
		ite != shared_objs.end();
		++ite)
	{
		// dummy
	}
}
