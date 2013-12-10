/*********************************************
 * tcpsession.cpp
 * Author: kamuszhou www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include "tcpsession.h"
#include "utils.h"

tcpsession::tcpsession() {
}

tcpsession::~tcpsession() {
}

void tcpsession::append_ip_sample(const unsigned char* ippkg)
{
	struct iphdr* iphdr; 
	mem_t mem;
	int len;
	
	assert(NULL!=ippkg);
	iphdr = (struct iphdr*)ippkg;
	len = ntohs(iphdr->tot_len);

	_ippkgs_samples.push_back(mem);
	_ippkgs_samples.back().cp(ippkg, len);
}

