/*********************************************
 * testsuite.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 24 Feb, 2014
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
	_done = true;
	_tester->join();

//	if (NULL != _pcap_handle)
//	{
//		pcap_close(_pcap_handle);
//	}
}

void testsuite::get_ready()
{
	_count_jobs = 0;

//	if (NULL == _pcap_handle)
//	{
//		_pcap_handle == pcap_open_dead(DLT_RAW, 65535 /* snaplen */);
//		assert(NULL != _pcap_handle);
//	}

	_tester.reset(
			new boost::thread(boost::bind(&testsuite::run_worker, this))
	);
}

void testsuite::report_sess_traffic(const std::string& client_src_ip,
						 uint16_t port,
						 const std::list<ip_pkt>& traffic,
						 tcpsession::cause_of_death cause)
{
	boost::shared_ptr<job_block> job(new job_block(client_src_ip, port, traffic, cause));
	bool success = _jobs.push(job);
	if (success)
	{
		_count_jobs++;
		if (_count_jobs == 1)
		{
			boost::mutex::scoped_lock lock(_mutex);
			_con_var.notify_one();
		}
	}
}

void testsuite::run_worker()
{
	bool success;
	boost::shared_ptr<job_block> job;

	load_lua_scripts();
	load_shared_objects();

	while (!_done)
	{
		success = _jobs.pop(job);
		if (success)
		{
			_count_jobs--;
			//std::cout << "consumed " << job->_client_str_ip << std::endl;
			//std::cout << _count_jobs << " left" << std::endl;

			// let's do the job.
			if (job->_cause == tcpsession::RESET)
			{
				continue;
			}

			if (job->_cause != tcpsession::PASSIVE_CLOSE && job->_cause != tcpsession::ACTIVE_CLOSE)
			{
				std::ostringstream ss;
				ss << boost::format("accident_death_%s_%d_%d.pcap") % job->_client_str_ip % job->_port % job->_cause;
				save_traffic(job->_traffic, ss.str());

				continue;
			}

			const std::list<ip_pkt>& traffic = job->_traffic;
			std::vector<char> request;
			std::vector<char> response;
			split_traffic(traffic, request, response);
			do_tests(job->_client_str_ip, job->_port, request, response);
		}
		else
		{
			boost::mutex::scoped_lock lock(_mutex);
			_con_var.timed_wait(lock, boost::posix_time::milliseconds(5000));
			//std::cout << "quit timed_wait" << std::endl;
		}
	}
}

void testsuite::split_traffic(const std::list<ip_pkt>& traffic, std::vector<char>& request, std::vector<char>& response)
{
	int request_traffic_size = 0;
	int response_traffic_size = 0;
	uint16_t server_port = g_configuration.get_dst_port();

	for (std::list<ip_pkt>::const_iterator ite = traffic.begin();
		 ite != traffic.end();
		 ++ite)
	{
		if (ite->get_src_port() == server_port )
		{
			response_traffic_size += ite->get_tcp_payload_len();
		}
		else
		{
			request_traffic_size += ite->get_tcp_payload_len();
		}
	}

	request.resize(request_traffic_size);
	response.resize(response_traffic_size);

	int response_copied_bytes = 0;
	int request_copied_bytes = 0;
	for (std::list<ip_pkt>::const_iterator ite = traffic.begin();
		 ite != traffic.end();
		 ++ite)
	{
		int payload_len = ite->get_tcp_payload_len();
		if (ite->get_src_port() == server_port )
		{
			memcpy(&response[0] + response_copied_bytes, ite->get_tcp_payload(), payload_len);
			response_copied_bytes += payload_len;
		}
		else
		{
			memcpy(&request[0] + request_copied_bytes, ite->get_tcp_payload(), payload_len);
			request_copied_bytes += payload_len;
		}
	}
}

void testsuite::save_traffic(const std::list<ip_pkt>& traffic, const std::string& pcap_file)
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
	static int most_saved_files_count = 64;

	if (0 == most_saved_files_count)
	{
		return;
	}
	most_saved_files_count--;

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

	os.write((const char*)&pcaphdr, sizeof(pcaphdr));

	for (std::list<ip_pkt>::const_iterator ite = traffic.begin();
		 ite != traffic.end();
		 ++ite)
	{
		int pkt_tot_len = ite->get_tot_len();
		memset(&pkthdr, 0, sizeof(pkthdr));
		// pkthdr.ts = 0;  // thie info is lost, i don't care. set it as zero.
		pkthdr.incl_len = pkt_tot_len;
		pkthdr.orig_len = pkt_tot_len;
		os.write((const char*)&pkthdr, sizeof(pkthdr));
		os.write(ite->get_starting_addr(), pkt_tot_len);

		//pcap_dump((u_char*)pdumper, &pkthdr, (const u_char*)ite->get_starting_addr());
	}

	os.close();

//	pcap_dump_close(pdumper);
}

void testsuite::do_tests(const std::string& client_str_ip, uint16_t port,
					     const std::vector<char>& request, const std::vector<char>& response)
{
	g_mylua.run_lua_tests(client_str_ip, request, response);
}

void testsuite::load_lua_scripts()
{
	std::list<std::string> scripts;

	std::string home = g_configuration.get_lua_scripts_home();
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

	std::string home = g_configuration.get_so_home();
	find_files(home, ".*\\.so$", shared_objs);

	for (std::list<std::string>::iterator ite = shared_objs.begin();
		ite != shared_objs.end();
		++ite)
	{
	}
}
