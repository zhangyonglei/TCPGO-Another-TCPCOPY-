/*********************************************
 * sessionmanager.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include <fstream>
#include "session_manager.h"
#include "utils.h"
#include "cute_logger.h"

using namespace std;

session_manager g_session_manager;

session_manager::session_manager()
{
}

int session_manager::read_from_capfile(const string& path, const string& filter)
{
	char ebuf[1024];
	pcap_t *pcap;
	const char *pkt_data;
	const char *ip_pkt;
	struct bpf_program fp;
	struct pcap_pkthdr pkt_hdr;
	struct timeval ts;
	uint16_t random;

	random = rand();

	if ((pcap = pcap_open_offline(path.c_str(), ebuf)) == NULL) {
		cerr << ebuf << endl;
		return -1;
	}

	if (!filter.empty()) {
		if (pcap_compile(pcap, &fp, filter.c_str(), 0, 0) == -1)
		{
			cerr << pcap_geterr(pcap) << endl;
			return -1;
		}
		if (pcap_setfilter(pcap, &fp) == -1)
		{
			pcap_freecode(&fp);
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
				ip_pkt = strip_l2head(pcap, pkt_data);
				ts = pkt_hdr.ts;
				if (ip_pkt != NULL)
				{
					uint16_t new_src_port;
					uint16_t ori_src_port;
					struct ip_pkt pkt(ip_pkt);
					ori_src_port = pkt.get_src_port();
					new_src_port = ((ori_src_port + random) % 30000) + 4096;
					pkt.modify_src_port(new_src_port);
					dispatch_ip_pkt(pkt.get_starting_addr());
				}
			}
		}
		else
		{
			break;
		}
	}
	pcap_close(pcap);

	clean();
}

int session_manager::dispatch_ip_pkt(const char* ip_pkt)
{
	int ret;
	uint64_t key;
	std::map<uint64_t, tcpsession>::iterator ite;
	std::pair<std::map<uint64_t, tcpsession>::iterator, bool>  ugly_pair;

	ip_packet_parser(ip_pkt);
	key = mk_sess_key(iphdr->saddr, tcphdr->source);

	tcpsession session(iphdr->saddr, tcphdr->source);
	// The following map::insert returns a pair, with its member pair::first set to an iterator pointing to
	// either the newly inserted element or to the element with an equivalent key in the map. The pair::second
	// element in the pair is set to true if a new element was inserted or false if an equivalent key already
	// existed. (copied from c++ references to clarify the obfuscated map::insert return value.)
	ugly_pair = _sessions.insert(std::pair<uint64_t, tcpsession>(key, session));
	ite = ugly_pair.first;
	ite->second.append_ip_sample((const char*)ip_pkt);
	if (ugly_pair.second)
	{
		g_postoffice.register_callback(key, &ite->second);
	}

	ret = 0;

	return ret;
}

int session_manager::clean()
{
	int healthy, total_sess_count, sick_sess_count;
	std::map<uint64_t, tcpsession>::iterator ite;

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
			g_postoffice.deregister_callback(ite->first);
			_sessions.erase(ite++);
		}

	}
	g_logger.printf("total %d sessions, %d of them are sick and are dropped.\n", total_sess_count, sick_sess_count);

	return 0;
}

int session_manager::get_ready()
{
	int count;
	count = 0;
	for (std::map<uint64_t, tcpsession>::iterator ite = _sessions.begin();
			ite != _sessions.end();
			++ite)
	{
		count++;
		ite->second.get_ready();
	}
	g_logger.printf("%d sessions are ready.\n", count);

	return 0;
}

void session_manager::remove_a_session(uint64_t key)
{
	int num;
	g_postoffice.deregister_callback(key);
	num = _sessions.erase(key);
	assert(num == 1);
}

session_manager::~session_manager()
{
}

