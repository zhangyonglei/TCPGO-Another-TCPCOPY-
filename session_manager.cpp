/*********************************************
 * sessionmanager.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include <fstream>
#include "session_manager.h"
#include "utils.h"
#include "cute_logger.h"

boost::mutex session_manager::_mutex;
std::vector<boost::shared_ptr<session_manager> > session_manager::_managers;

session_manager::session_manager(int asio_idx)
{
	_asio_idx = asio_idx;
}

session_manager::~session_manager()
{
}

session_manager& session_manager::instance(int idx)
{
	if (0 == _managers.size())
	{
		boost::lock_guard<boost::mutex> lock(_mutex);
		if (0 == _managers.size())
		{
			_managers.resize(g_configuration.get_asio_thrd_num());
			for (int i = 0; i < _managers.size(); i++)
			{
				_managers[i].reset(new session_manager(i));
			}
		}
	}

	return *_managers[idx].get();
}

int session_manager::read_from_pcapfile(const std::string& path, const std::string& filter)
{
	char ebuf[1024];
	pcap_t *pcap;
	const char *pkt_data;
	const char *ippkt;
	struct bpf_program fp;
	struct pcap_pkthdr pkt_hdr;
	struct timeval ts;

	if ((pcap = pcap_open_offline(path.c_str(), ebuf)) == NULL)
	{
		g_logger.printf("%s\n", ebuf);
		return -1;
	}

	g_logger.printf("read traffic from file: %s\n", path.c_str());

	if (!filter.empty())
	{
		// some old version libpcap library declares the third parameter of pcap_compile as type of char*
		// rather than const char*. To pass the compiling on this case, a cast has to be made.
		if (pcap_compile(pcap, &fp, (char*)filter.c_str(), 0, 0) == -1)
		{
			std::cerr << pcap_geterr(pcap) << std::endl;
			pcap_close(pcap);
			return -1;
		}
		if (pcap_setfilter(pcap, &fp) == -1)
		{
			pcap_freecode(&fp);
			pcap_close(pcap);
			return -1;
		}
	}
	pcap_freecode(&fp);

	while (true)
	{
		int truncated_pkt_count = 0;
		pkt_data = (const char*)pcap_next(pcap, &pkt_hdr);
		if (pkt_data != NULL)
		{
			if (pkt_hdr.caplen < pkt_hdr.len)
			{
				g_logger.printf("%d truncated packets are detected.\n",
						++truncated_pkt_count);
				continue;
			}
			else
			{
				ippkt = strip_l2head(pcap, pkt_data);
				ts = pkt_hdr.ts;
				if (ippkt != NULL)
				{
					uint16_t new_src_port;
					uint16_t ori_src_port;
					int asio_idx;

					boost::shared_ptr<ip_pkt> pkt = boost::make_shared<ip_pkt>(ippkt);
					ori_src_port = pkt->get_src_port();
					new_src_port = generate_the_port(ori_src_port);
					pkt->modify_src_port(new_src_port);
					asio_idx = pkt->get_asio_idx_outbound();
					instance(asio_idx).dispatch_ip_pkt(pkt);
				}
			}
		}
		else
		{
			break;
		}
	}
	pcap_close(pcap);

	for (int i = 0; i < g_configuration.get_asio_thrd_num(); i++)
	{
		instance(i).clean_sick_session();
	}

	return 0;
}

void session_manager::dispatch_ip_pkt(boost::shared_ptr<ip_pkt> pkt)
{
	uint64_t key;
	SessMap::iterator ite;
	std::pair<SessMap::iterator, bool> ugly_pair;

	key = pkt->get_sess_key_outbound();

	tcpsession session(pkt->get_asio_idx_outbound(), pkt->get_iphdr()->saddr, pkt->get_tcphdr()->source);
	// The following map::insert returns a pair, with its member pair::first set to an iterator pointing to
	// either the newly inserted element or to the element with an equivalent key in the map. The pair::second
	// element in the pair is set to true if a new element was inserted or false if an equivalent key already
	// existed. (copied from c++ references to clarify the obfuscated map::insert return value.)
	ugly_pair = _sessions.insert(std::pair<uint64_t, tcpsession>(key, session));
	ite = ugly_pair.first;
	ite->second.append_ip_sample(pkt);
	if (ugly_pair.second)
	{
		postoffice::instance(_asio_idx).register_callback(key, &ite->second);
	}
}

void session_manager::inject_a_realtime_ippkt(boost::shared_ptr<ip_pkt> pkt)
{
	static uint64_t ip_count;
	uint64_t key;
	uint16_t src_port;  // host byte order
	SessMap::iterator ite;
	std::pair<SessMap::iterator, bool> ugly_pair;
	int total_count = _sessions.size();

	// hard code the session count ceiling
	if (total_count >= _session_count_limit)
	{
		_traffic_jam = true;
	}
	else if (_traffic_jam)
	{
		if (_healthy_sess_count <= _expected_qps * 3)
		{
			_traffic_jam = false;
		}
	}

	src_port = ntohs(pkt->get_tcphdr()->source);
	// the following logic ignore product server's outgoing traffic
	// if the user by mistake did this: tcpdump -i any src 80 -s 0 -w - | netcat xxx.xxx.xxx.xxx 1993
	if (src_port < 1024)
	{
		return;
	}

	key = pkt->get_sess_key_outbound();

	tcpsession session(pkt->get_asio_idx_outbound(), pkt->get_iphdr()->saddr, pkt->get_tcphdr()->source);
	ugly_pair = _sessions.insert(std::pair<uint64_t, tcpsession>(key, session));
	if (ugly_pair.second)  // a new tcpsession is created and added.
	{
		// traffic control is imposed. New tcpsession is now allowed.
		if (_traffic_jam)
		{
			_sessions.erase(key);
			return;
		}
		ugly_pair.first->second.get_ready_for_rt_traffic();
	}
	else
	{
		// traffic is bound for already existed tcpsession.
	}

	ite = ugly_pair.first;
	ite->second.inject_a_realtime_ippkt(pkt);
}

int session_manager::clean_sick_session()
{
	int healthy, total_sess_count, sick_sess_count;
	SessMap::iterator ite;

	total_sess_count = 0;
	sick_sess_count = 0;

	for (ite = _sessions.begin(); ite != _sessions.end();)
	{
		total_sess_count++;
		healthy = ite->second.check_samples_integrity();
		if (healthy == 0)
		{
			++ite;
		}
		else
		{
			sick_sess_count++;
			postoffice::instance(_asio_idx).deregister_callback(ite->first);
			_sessions.erase(ite++);
		}

	}
	g_logger.printf("session_manager[%d] has %d sessions, %d of them are sick and are dropped. %d sessions are healthy.\n",
			_asio_idx, total_sess_count, sick_sess_count, total_sess_count - sick_sess_count);

	return 0;
}

int session_manager::get_ready()
{
	int count;
	count = 0;

	_healthy_sess_count = 0;
	_traffic_jam = false;
	_session_count_limit = g_configuration.get_session_count_limit();
	_expected_qps = g_configuration.get_expected_qps();

	// there are some preloaded offline traffic.
	for (SessMap::iterator ite = _sessions.begin();
			ite != _sessions.end(); ++ite)
	{
		count++;
		ite->second.get_ready_for_offline_traffic();
	}
	g_logger.printf("%d sessions are ready at current.\n", count);

	return 0;
}

void session_manager::erase_a_session(uint64_t key)
{
	int num;
	num = _sessions.erase(key);
	assert(num == 1);
}
