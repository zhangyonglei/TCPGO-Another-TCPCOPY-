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

	_pkt = new char[pkt._tot_len];
	assert(_pkt != NULL);
	memcpy((char*)_pkt, pkt._pkt, pkt._tot_len);
	warm_up();
}

ip_pkt::ip_pkt(const char* pkt)
{
	_pkt = NULL;
	cp(pkt);
}

ip_pkt::~ip_pkt()
{
	delete _pkt;
	_pkt = NULL;
}

const ip_pkt& ip_pkt::operator=(const ip_pkt& ip_pkt)
{
	cp(ip_pkt._pkt);
}

bool ip_pkt::operator<(const ip_pkt& challenger)const
{
	bool b;
	b = seq_before(_seq, challenger._seq);
	if (b)
	{
		return true;
	}

	if (_seq == challenger._seq)
	{
		b = seq_before(_ack_seq, challenger._ack_seq);
		if (b)
		{
			return true;
		}

		b = challenger._rst_flag - _rst_flag;
		if (b)
		{
			return true;
		}

		b = challenger._fin_flag - _fin_flag;
		return b;
	}

	return false;
}

bool ip_pkt::operator>(const ip_pkt& challenger)const
{
	bool b;
	b = seq_after(_seq, challenger._seq);
	return b;
}

bool ip_pkt::operator==(const ip_pkt& ip_pkt)const
		{
	if (!(*this < ip_pkt) && !(ip_pkt < *this))
		return true;
	else
		return false;
		}

void ip_pkt::cp(const char* pkt)
{
	assert(NULL != pkt);
	ip_packet_parser(pkt);
	delete []_pkt;
	_pkt = new char[ip_tot_len];
	assert(NULL != _pkt);
	memcpy((char*)_pkt, pkt, ip_tot_len);
	warm_up();
}

void ip_pkt::swap(ip_pkt& pkt)
{
	char* tmp;

	tmp = _pkt;
	_pkt = pkt._pkt;
	pkt._pkt = tmp;

	warm_up();
	pkt.warm_up();
}

void ip_pkt::warm_up()
{
	if(NULL == _pkt)
		return;

	ip_packet_parser(_pkt);

	_tot_len = ip_tot_len;
	_iphdr = (struct iphdr*)iphdr;
	_ip_content = _pkt + iphdr_len;
	_iphdr_len = iphdr_len;

	_tcphdr = (struct tcphdr*)tcphdr;
	_tcphdr_len = tcphdr_len;
	_tcp_payload = (char*)tcp_payload;
	_tcp_payload_len = tcp_payload_len;

	_seq = ntohl(tcphdr->seq);
	_ack_seq = ntohl(tcphdr->ack_seq);
	_win_size = ntohs(tcphdr->window);

	_ack_flag = _tcphdr->ack;
	_rst_flag = _tcphdr->rst;
	_syn_flag = _tcphdr->syn;
	_fin_flag = _tcphdr->fin;

	struct in_addr inaddr;
	inaddr.s_addr = _iphdr->saddr;
	_src_addr = inet_ntoa(inaddr);
	inaddr.s_addr = _iphdr->daddr;
	_dst_addr = inet_ntoa(inaddr);
}

void ip_pkt::modify_src_port(uint16_t src_port)
{
	_tcphdr->source = htons(src_port);
	warm_up();
}

uint16_t ip_pkt::reset_tcp_checksum()
{
	uint16_t old_checksum, new_checksum;
	char *ptr;

	ptr = (char*)_tcphdr + 16;  // 16 is the offset of checksum in tcp header
	old_checksum = *(uint16_t*)(ptr);
	memset(ptr, 0, 2);
	new_checksum = calc_tcp_checksum(_iphdr, _tcphdr);
	memcpy(ptr, &new_checksum, 2);

	return old_checksum;
}

uint16_t ip_pkt::reset_ip_checksum()
{
	uint16_t old_checksum, new_checksum;
	char *ptr;

	ptr = (char*)_iphdr + 10; // 10 is the offset of checksum in ip header
	old_checksum = *(uint16_t*)(ptr);
	memset(ptr, 0, 2);
	new_checksum = calc_ip_checksum(_iphdr);
	memcpy(ptr, &new_checksum, 2);
}

void ip_pkt::rebuild(const char* addr, unsigned short port, uint32_t expected_next_sequence_from_peer)
{
	int ret;
	struct in_addr inaddr;
	memset(&inaddr, 0, sizeof(inaddr));
	ret = inet_aton(addr, &inaddr);
	if (ret < 0)
	{
		perror(addr);
		abort();
	}

	_iphdr->daddr = *(uint32_t*)&inaddr;
	_tcphdr->dest = htons(port);
	_tcphdr->window = htons(65535);
	_tcphdr->ack_seq = htonl(expected_next_sequence_from_peer);
	warm_up();
	reset_ip_checksum();
	reset_tcp_checksum();
}
