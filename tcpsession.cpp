/*********************************************
 * tcpsession.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include "tcpsession.h"
#include "utils.h"
#include "cute_logger.h"

tcpsession::tcpsession(uint32_t ip, uint16_t port) {
	struct in_addr inaddr;
	inaddr.s_addr = ip;

    _client_src_ip_num = ip;
    _client_src_ip_str = inet_ntoa(inaddr);
	_client_src_port = ntohs(port);
}

tcpsession::~tcpsession() {
}

void tcpsession::append_ip_sample(const unsigned char* ippkt)
{
	struct iphdr* iphdr; 
	ip_pkt pkt;
	int len;
	
	assert(NULL!=ippkt);
	iphdr = (struct iphdr*)ippkt;
	len = ntohs(iphdr->tot_len);

	_ippkts_samples.push_back(pkt);
	_ippkts_samples.back().cp(ippkt);
}

int tcpsession::check_health()
{
	int ret;
	int size_saved, size_now;
	size_saved = _ippkts_samples.size();
	_ippkts_samples.sort();
	_ippkts_samples.unique();
	size_now = _ippkts_samples.size();

	ret = 0;
	if (size_now != size_saved)
	{
		g_logger.printf("tcpsession:%s:%hu has %d duplicated packets dropped.\n",
				_client_src_ip_str.c_str(), _client_src_port, size_saved - size_now);
		ret = 1;
	}

	return ret;
}


