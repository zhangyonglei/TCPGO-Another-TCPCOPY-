/*********************************************
 * ippkg.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: 12 Dec, 2013
 ********************************************/

#include "ip_pkt.h"
#include "utils.h"

ip_pkt::ip_pkt()
{
	_pkt = NULL;
}

ip_pkt::ip_pkt(const ip_pkt& pkt)
{
	if (NULL == pkt._pkt)
	{
		_pkt = NULL;
		return;
	}

	_pkt = new unsigned char[pkt._tot_len];
	assert(_pkt != NULL);
	memcpy(_pkt, pkt._pkt, pkt._tot_len);
	warm_up();
}

ip_pkt::ip_pkt(const unsigned char* ip_pkt)
{
	_pkt = NULL;
	cp(ip_pkt);
}

ip_pkt::~ip_pkt()
{
	delete _pkt;
}

const ip_pkt& ip_pkt::operator=(const ip_pkt& ip_pkt)
{
	cp(ip_pkt._pkt);
}

bool ip_pkt::operator<(const ip_pkt& challenger)const
{
	bool b;
	b = seq_before(_seq, challenger._seq);
	return b;
}

void ip_pkt::cp(const unsigned char* ip_pkt)
{
	assert(NULL != ip_pkt);
	ip_packet_parser(ip_pkt);
	delete []_pkt;
	_pkt = new unsigned char[ip_tot_len];
	assert(NULL != _pkt);
	memcpy(_pkt, ip_pkt, ip_tot_len);
	warm_up();
}

void ip_pkt::warm_up()
{
	ip_packet_parser(_pkt);

	_tot_len = ip_tot_len;
	_iphdr = iphdr;
	_ip_content = (const unsigned char*)(_pkt + iphdr_len);
	_ihl = iphdr_len;

	_tcphdr = tcphdr;
	_tcp_content = tcp_content;

	_seq = ntohl(tcphdr->seq);
	_ack_seq = ntohl(tcphdr->ack_seq);

	_ack_flag = _tcphdr->ack;
	_rst_flag = _tcphdr->rst;
	_syn_flag = _tcphdr->syn;
	_fin_flag = _tcphdr->fin;
}
