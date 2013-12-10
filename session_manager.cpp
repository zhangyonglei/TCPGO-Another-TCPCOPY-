/*********************************************
 * sessionmanager.cpp
 * Author: kamuszhou www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include <fstream>
#include "session_manager.h"
#include "utils.h"

using namespace std;

session_manager::session_manager()
{
}

int session_manager::read_from_capfile(const string& path, const string& filter)
{
	char  ebuf[1024];
	pcap_t  *pcap;
	const u_char *pkt_data;
	const u_char *ip_data;
	struct bpf_program  fp;
	struct pcap_pkthdr  pkt_hdr;
	struct timeval ts;

	if ((pcap = pcap_open_offline(path.c_str(), ebuf)) == NULL) {
		cerr << ebuf <<endl;
		return -1;
	}

	if (!filter.empty()) {
		if (pcap_compile(pcap, &fp, filter.c_str(), 0, 0) == -1) {
			return -1;
		}
		if (pcap_setfilter(pcap, &fp) == -1) {
			pcap_freecode(&fp);
			return -1;
		}
	}
	pcap_freecode(&fp);

	while (true) {
		pkt_data = pcap_next(pcap, &pkt_hdr);
		if (pkt_data != NULL) {
			if (pkt_hdr.caplen < pkt_hdr.len) {
				continue;
			} else {
			        ip_data = strip_l2head(pcap, pkt_data);
				ts = pkt_hdr.ts;
				if (ip_data != NULL) {

				}
			}
		} else {
			break;
		}
	}
	pcap_close(pcap);
}

session_manager::~session_manager()
{
}

