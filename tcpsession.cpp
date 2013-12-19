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

	get_ready();
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

void tcpsession::get_ready()
{
	std::list<ip_pkt>::iterator tmp_ite;

	_current_state = tcpsession::CLOSED;
	_expected_next_sequence_from_peer = 0;
	_latest_acked_sequence_by_peer = 0;
	_advertised_window_size = 0;
	_sliding_window_left_boundary = _ippkts_samples.begin();
	tmp_ite = _sliding_window_left_boundary;
	++tmp_ite;
	_sliding_window_right_boundary = tmp_ite;
}

int tcpsession::pls_send_these_packets(std::vector<const ip_pkt*>& pkts)
{
	ip_pkt* pkt;

	pkts.clear();
	for(std::list<ip_pkt>::iterator ite = _sliding_window_left_boundary;
		ite != _sliding_window_right_boundary;
		++ite)
	{
		pkt = &(*ite);
		// pkt->rebuild("127.0.0.1", 80);  // failed to work.
		pkt->rebuild("192.168.44.129", 80); // TODO hard code temporarily.
		pkts.push_back(pkt);
	}

	return pkts.size();
}

void tcpsession::got_a_packet(const ip_pkt* pkt)
{
	switch(_current_state)
	{
	case CLOSED:
		closed_state_handler(pkt);
		break;

	case LISTEN: 
		listen_state_handler(pkt);
		break;

	case SYN_RCVD: 
		syn_rcvd_state_handler(pkt);
		break;
		
	case SYN_SENT:
		syn_sent_state_handler(pkt);
		break;

	case ESTABLISHED: 
		established_state_handler(pkt);
		break;

	case CLOSE_WAIT:
		close_wait_state_handler(pkt);
		break;

	case LAST_ACK:
		last_ack_state_handler(pkt);
		break;
		
	case FIN_WAIT_1:
		fin_wait_1_state_handler(pkt);
		break;

	case FIN_WAIT_2:
		fin_wait_2_state_handler(pkt);
		break;

	case CLOSING:
		closing_state_handler(pkt);
		break;

	case TIME_WAIT:
		time_wait_state_handler(pkt);
		break;

	default:
		// catch ya. only god and bug knows how to reach here.
		abort();
	}
}

void tcpsession::closed_state_handler(const ip_pkt* pkt)
{
	// do nothing, it's assumed the peer will got a time out event finally.
}

void tcpsession::listen_state_handler(const ip_pkt* pkt)
{
	// not supposed to reach here.
	abort();
}

void tcpsession::syn_rcvd_state_handler(const ip_pkt* pkt)
{
	// this event rarely happens in real world. i'm not planning to support this.
	// so, code will never get here.
}

void tcpsession::syn_sent_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_syn_set() && pkt->is_ack_set())
	{
		_current_state = tcpsession::ESTABLISHED;
	}
	refresh_status(pkt);
}

void tcpsession::established_state_handler(const ip_pkt* pkt)
{
}

void tcpsession::close_wait_state_handler(const ip_pkt* pkt)
{
}

void tcpsession::last_ack_state_handler(const ip_pkt* pkt)
{
}

void tcpsession::fin_wait_1_state_handler(const ip_pkt* pkt)
{
}

void tcpsession::fin_wait_2_state_handler(const ip_pkt* pkt)
{
}

void tcpsession::closing_state_handler(const ip_pkt* pkt)
{
}

void tcpsession::time_wait_state_handler(const ip_pkt* pkt)
{
}

void tcpsession::refresh_status(const ip_pkt* pkt)
{
	uint32_t seq;
	uint32_t ack_seq;
	uint32_t ack_seq_tmp;
	uint16_t win_size_saved;
	
	std::list<ip_pkt>::iterator ite;

	ip_packet_parser(pkt);
	seq = ntohl(tcphdr->seq);
	ack_seq = ntohl(tcphdr->ack_seq);

	if (pkt->is_syn_set() && pkt->is_ack_set())
	{
		_expected_next_sequence_from_peer = pkt->get_seq() + 1;
		_latest_acked_sequence_by_peer = pkt->get_ack_seq();
		_advertised_window_size = pkt->get_win_size();
		return;
	}

	if (seq + 1 == _expected_next_sequence_from_peer)
	{
		_expected_next_sequence_from_peer = pkt->get_seq() + pkt->get_tcp_content_len();
	}

	if (pkt->is_ack_set())
	{
		ack_seq_tmp = pkt->get_ack_seq();
		if (seq_before(_latest_acked_sequence_by_peer, ack_seq_tmp))
		{
			_latest_acked_sequence_by_peer = pkt->get_ack_seq();
		}
	}

	// eliminate  acked packets.
	for (ite = _sliding_window_left_boundary;
		 ite != _sliding_window_right_boundary;)
	{
		if (seq_before(ite->get_seq(), _latest_acked_sequence_by_peer))
		{
			_ippkts_samples.erase(ite++);
			_sliding_window_left_boundary = ite;
		}
		else
		{
			break;
		}
	}

	win_size_saved = _advertised_window_size;
	_advertised_window_size = pkt->get_win_size();

	// adjust sliding window
	int win_size = 0;
	int ippkt_count_walked_through = 0;
	for (ite = _sliding_window_left_boundary;
		 ite != _sliding_window_right_boundary;)
	{
		win_size += ite->get_tot_len();
		if (win_size > _advertised_window_size)
		{
			// make sure at least one IP packet are available to be sent.
			if(0 == ippkt_count_walked_through)
			{
				break;
			}
			else  // reduce the window size.
			{
				_sliding_window_right_boundary = ite;
				break;
			}
		}
		ippkt_count_walked_through++;
	}
	if (win_size < _advertised_window_size);
	{
		// try to increase the window size
		for(; ite != _ippkts_samples.end(); ++ite)
		{
			win_size += ite->get_tot_len();
			if (win_size > _advertised_window_size)
			{
				break;
			}
		}
		_sliding_window_right_boundary = ite;
	}
}
