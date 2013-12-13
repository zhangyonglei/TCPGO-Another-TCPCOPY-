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

session_manager::session_manager()
{
}

int session_manager::read_from_capfile(const string& path, const string& filter)
{
	char  ebuf[1024];
	pcap_t  *pcap;
	const u_char *pkt_data;
	const u_char *ip_pkt;
	struct bpf_program  fp;
	struct pcap_pkthdr  pkt_hdr;
	struct timeval ts;

	if ((pcap = pcap_open_offline(path.c_str(), ebuf)) == NULL) {
		cerr << ebuf <<endl;
		return -1;
	}

	if (!filter.empty()) {
		if (pcap_compile(pcap, &fp, filter.c_str(), 0, 0) == -1) {
			cerr << pcap_geterr(pcap) <<endl;
			return -1;
		}
		if (pcap_setfilter(pcap, &fp) == -1) {
			pcap_freecode(&fp);
			return -1;
		}
	}
	pcap_freecode(&fp);

	while (true) {
		int truncated_pkt_count = 0;
		pkt_data = pcap_next(pcap, &pkt_hdr);
		if (pkt_data != NULL) {
			if (pkt_hdr.caplen < pkt_hdr.len) {
				g_logger.printf("%d truncated packets are detected.\n", ++truncated_pkt_count);
				continue;
			} else {
			        ip_pkt = strip_l2head(pcap, pkt_data);
				ts = pkt_hdr.ts;
				if (ip_pkt != NULL) {
						dispatch_ip_pkt(ip_pkt);
				}
			}
		} else {
			break;
		}
	}
	pcap_close(pcap);
}

int session_manager::dispatch_ip_pkt(const u_char* ip_pkt)
{
	int ret;
	uint64_t key;
	ip_packet_parser(ip_pkt);
	key = mk_sess_key(iphdr->saddr, tcphdr->dest);

	tcpsession& session = _sessions[key];
	session.append_ip_sample(ip_pkt);

	return 0;
}

int session_manager::loop()
{
	std::map<uint64_t, tcpsession>::iterator ite;
	for (ite = _sessions.begin(); ite != _sessions.end(); ++ite)
	{
		ite->second.verify();
	}
}

session_manager::~session_manager()
{
}

