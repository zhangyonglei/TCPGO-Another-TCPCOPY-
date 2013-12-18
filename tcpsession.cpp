/*********************************************
 * tcpsession.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include "tcpsession.h"
#include "utils.h"
#include "cute_logger.h"

tcpsession::tcpsession(uint32_t ip, uint16_t port)
{
	struct in_addr inaddr;
	inaddr.s_addr = ip;

	_client_src_ip_num = ip;
	_client_src_ip_str = inet_ntoa(inaddr);
	_client_src_port = ntohs(port);
}

tcpsession::~tcpsession()
{
}

void tcpsession::append_ip_sample(const char* ippkt)
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
	// reomve the packet without tcp playload. e.g. the tcp keep-alive segments which are
	// used to elicit an ACK from the receiver.
	for(ite = _ippkts_samples.begin(); ite != _ippkts_samples.end();)
	{
		int tot_len = ite->get_tot_len();
		int tcp_content_len = ite->get_tcp_content_len();
		int iphdr_len = ite->get_iphdr_len();
		int tcphdr_len = ite->get_tcphdr_len();
		bool fin_set = ite->is_fin_set();
		bool ack_set = ite->is_ack_set();
		bool sin_set = ite->is_syn_set();
		bool rst_set = ite->is_rst_set();

		// remove usefuless samples
		if (0 == tcp_content_len && !fin_set && !rst_set && ack_set)
		{
			_ippkts_samples.erase(ite++);
		}
		// remove  corrupted sample, this case occurs rarely.
		else if (tot_len != iphdr_len + tcphdr_len + tcp_content_len)
		{
			std::cerr << "detected corrupted ip packet." << ite->get_src_addr() << " : " << ite->get_src_port()
					<< " --> " <<ite->get_dst_addr() << " : " << ite->get_dst_port() << std::endl;
			++ite;
		}
		else
		{
			++ite;
		}
	}

	size_saved = _ippkts_samples.size();
	if (0 == size_saved)
	{
		goto _err;
	}
	_ippkts_samples.sort();
	_ippkts_samples.unique();   // remove duplicates.
	size_now = _ippkts_samples.size();
	if (size_now != size_saved)
	{
	//	g_logger.printf("tcpsession:%s:%hu has %d duplicated packets dropped.\n",
	//			_client_src_ip_str.c_str(), _client_src_port, size_saved - size_now);
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
	}

	return 0;

_err:
	return 1;
}

const ip_pkt* tcpsession::pls_send_this_packet()
{
	ip_pkt* pkt;
	if (_ite_next_avaliable == _ippkts_samples.end())
		return NULL;
	_ite_next_avaliable = _ippkts_samples.begin();

	// testing code .......
	for(; _ite_next_avaliable!=_ippkts_samples.end(); ++_ite_next_avaliable)
	{
		pkt = &(*_ite_next_avaliable);
		// testing code ...
		//if (pkt->get_seq() == 2345627925ul && pkt->get_dst_port() == 80)
		{
			//_ite_next_avaliable->rebuild("127.0.0.1", 80);
			pkt->rebuild("192.168.44.129", 80);
			return pkt;
		}
	}

	return NULL;
}

void tcpsession::got_a_packet(const ip_pkt *pkt)
{
	// testing code.
	_ite_next_avaliable = _ippkts_samples.begin();
	/////////////////////////

	std::cout << "got_a_packet seq:" << pkt->get_seq() << "ackseq:" << pkt->get_ack_seq() << std::endl;
}


