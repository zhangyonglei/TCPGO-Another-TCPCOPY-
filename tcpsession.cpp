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

void tcpsession::append_ip_sample(const uint8_t* ippkt)
{
	struct iphdr* iphdr; 
	ip_pkt pkt;
	int32_t len;

	assert(NULL!=ippkt);
	iphdr = (struct iphdr*)ippkt;
	len = ntohs(iphdr->tot_len);

	_ippkts_samples.push_back(pkt);
	_ippkts_samples.back().cp(ippkt);
}

int32_t tcpsession::check_samples_integrity()
{
	int32_t size_saved, size_now;
	int32_t i;   // for the convenience of debug.
	uint32_t seq;
	uint32_t expected_next_seq;
	int32_t tcp_content_len;

	i = 0;
	std::list<ip_pkt>::iterator ite;
	// reomve the packet without tcp playload.
	for(ite = _ippkts_samples.begin(); ite != _ippkts_samples.end();)
	{
		if(0 == ite->get_tcp_content_len() && !ite->is_fin_set() && ite->is_ack_set()
				&& !ite->is_syn_set() && !ite->is_rst_set())
		{
			_ippkts_samples.erase(ite++);
		}
		else
		{
			++ite;
		}
	}

	size_saved = _ippkts_samples.size();
	if (0 == size_saved)
		goto _err;
	_ippkts_samples.sort();
	_ippkts_samples.unique();   // remove duplicates.
	size_now = _ippkts_samples.size();
	if (size_now != size_saved)
	{
		g_logger.printf("tcpsession:%s:%hu has %d duplicated packets dropped.\n",
				_client_src_ip_str.c_str(), _client_src_port, size_saved - size_now);
	}

	ite = _ippkts_samples.begin();
	// the first packet is not a syn, so these samples are considered as incomplete.
	if (!ite->is_syn_set())
	{
		goto _err;
	}
	expected_next_seq = ite->get_seq() + 1;
	++ite;
	++i;
	for (; ite != _ippkts_samples.end(); ++ite, ++i)
	{
		seq = ite->get_seq();
		if(expected_next_seq != seq)
		{
			// The last IP packet has rst set. In this case, the seq may be the last seq plus one.
			// But in most cases as I observed, it doesn't increase the last seq.
			if (i + 1 == size_now && ite->is_rst_set() && expected_next_seq + 1 == seq)
			{
				_ippkts_samples.erase(++ite, _ippkts_samples.end());
				return 0;
			}
			else
			{
				goto _err;
			}
		}
		tcp_content_len = ite->get_tcp_content_len();
		if (tcp_content_len > 0)
		{
			expected_next_seq += tcp_content_len;
		}

		if(ite->is_fin_set() || ite->is_rst_set())
		{
			_ippkts_samples.erase(++ite, _ippkts_samples.end());
			return 0;
		}

		///////////////////////// testing code for debug ///////////////
		//		if (ite->get_tcp_content_len() != 0 && ite->is_fin_set())
		//		{
		//			size_saved = size_now;
		//			std::cout <<"aaaaaaaaaaaaaaaaaaaa\n";
		//		}
	}

	return 0;

_err:
	g_logger.printf("samples in tcpsession:%s:%hu are not consecutive.\n",
			_client_src_ip_str.c_str(), _client_src_port);
	return 1;
}


