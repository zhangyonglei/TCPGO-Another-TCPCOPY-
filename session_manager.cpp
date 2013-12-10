/*********************************************
 * sessionmanager.cpp
 * Author: kamuszhou www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include <fstream>
#include "session_manager.h"
#include "utils.h"
#include "cutelogger.h"

using namespace std;

session_manager::session_manager()
{
}

int session_manager::read_from_capfile(const string& path, const string& filter)
{
	char  ebuf[1024];
	pcap_t  *pcap;
	const u_char *pkt_data;
	const u_char *ip_pkg;
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
		int truncated_pkg_count = 0;
		pkt_data = pcap_next(pcap, &pkt_hdr);
		if (pkt_data != NULL) {
			if (pkt_hdr.caplen < pkt_hdr.len) {
				g_logger.printf("%d truncated packages are detected.\n", ++truncated_pkg_count);
				continue;
			} else {
			        ip_pkg = strip_l2head(pcap, pkt_data);
				ts = pkt_hdr.ts;
				if (ip_pkg != NULL) {
						dispatch_ip_pkg(ip_pkg);
				}
			}
		} else {
			break;
		}
	}
	pcap_close(pcap);
}

int session_manager::dispatch_ip_pkg(const u_char* ip_pkg)
{
	int ret;
	uint64_t key;
	ip_package_parser(ip_pkg);
	key = mk_sess_key(iphdr->saddr, tcphdr->dest);

	tcpsession& session = _sessions[key];
	session.append_ip_sample(ip_pkg);

	return 0;
}

session_manager::~session_manager()
{
}

