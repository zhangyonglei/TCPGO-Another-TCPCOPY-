/*********************************************
 * tcpsession.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include "tcpsession.h"
#include "utils.h"

tcpsession::tcpsession() {
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

int tcpsession::verify()
{
	_ippkts_samples.sort();
}


