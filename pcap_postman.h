/*********************************************
 * pcap_postman.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Mar, 2014
 ********************************************/

#ifndef _PCAP_POSTMAN_H_
#define _PCAP_POSTMAN_H_

#include "postman.h"

class pcap_postman : public postman
{
public:
	pcap_postman(postoffice* po);
	virtual ~pcap_postman();

private:
	virtual void get_ready4subclass();
	virtual void recv_impl();

private:
	friend void pcaphandler(unsigned char* user, const pcap_pkthdr* hdr, const unsigned char* bytes);
	struct bpf_program _filter;
	pcap_t* _pcap_handle;
	char _errbuf[PCAP_ERRBUF_SIZE];
	int _truncated_pkt_count;
};

#endif /* _PCAP_POSTMAN_H_ */
