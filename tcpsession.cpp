/*********************************************
 * tcpsession.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include "tcpsession.h"
#include "utils.h"
#include "cute_logger.h"
#include "thetimer.h"
#include "session_manager.h"

tcpsession::tcpsession(uint32_t ip, uint16_t port)
{
	struct in_addr inaddr;
	inaddr.s_addr = ip;

	_client_src_ip_num = ip;
	_client_src_ip_str = inet_ntoa(inaddr);
	_client_src_port = ntohs(port);
	_session_key = mk_sess_key(ip, port);

	struct iphdr *iphdr = (struct iphdr*)_ack_template;
	struct tcphdr *tcphdr = (struct tcphdr*)(_ack_template + 20);
	memset(_ack_template, 0, 40);
	iphdr->ihl = 5;
	iphdr->version = 4;
	iphdr->tos = 0;
	iphdr->tot_len = htons(40);
    iphdr->id = 0;
    iphdr->frag_off = 0;
    iphdr->ttl = 255;
    iphdr->protocol = IPPROTO_TCP;
    iphdr->check = 0;
    iphdr->saddr = ip;
    iphdr->daddr = 0;

	tcphdr->source = port;
    tcphdr->dest = 0;
    tcphdr->seq = 0;
    tcphdr->ack_seq = 0;
    tcphdr->ack = 0;
    tcphdr->doff = 5;
    tcphdr->window = 65535;
    tcphdr->check = 0;
    tcphdr->urg_ptr = 0;
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
	_expected_last_ack_seq_from_peer = 0;
	_advertised_window_size = 0;
	_sliding_window_left_boundary = _ippkts_samples.begin();
	tmp_ite = _sliding_window_left_boundary;
	++tmp_ite;
	_sliding_window_right_boundary = tmp_ite;
	last_recorded_recv_time = g_timer.get_jiffies();
	last_recorded_snd_time = g_timer.get_jiffies();

	g_logger.printf("session %s.%hu is ready.\n", _client_src_ip_str.c_str(), _client_src_port);
}

int tcpsession::pls_send_these_packets(std::vector<const ip_pkt*>& pkts)
{
	ip_pkt* pkt;
	int count;
	uint64_t jiffies;

	jiffies = g_timer.get_jiffies();

	pkts.clear();

	// don't send too quickly.
	if (jiffies - last_recorded_snd_time <= 2)
	{
		return 0;
	}

	for(std::list<ip_pkt>::iterator ite = _sliding_window_left_boundary;
		ite != _sliding_window_right_boundary;
		++ite)
	{
		pkt = &(*ite);
		// pkt->rebuild("127.0.0.1", 80);  // failed to work.
		// TODO hard code the dstination IP and port temporarily.
		pkt->rebuild("192.168.44.129", 80, _expected_next_sequence_from_peer);
		pkts.push_back(pkt);
	}

	count = pkts.size();
	if (0 != count && pkts[0]->is_syn_set())
	{
		assert(1 == count);
		_current_state = tcpsession::SYN_SENT;
		g_logger.printf("move to state SYN_SENT\n");
		last_recorded_recv_time = jiffies;
	}
	else
	{
		// hard code the timeout value as one second.
		if (jiffies - last_recorded_recv_time > 3*HZ)  // timeout
		{
			g_logger.printf("session: %s.%hu time out.\n", _client_src_ip_str.c_str(), _client_src_port);
			g_session_manager.remove_a_session(_session_key);
		}
	}

	if (0 != count && pkts[count-1]->is_fin_set())
	{
		const ip_pkt* pkt = pkts[count-1];
		if (_current_state == tcpsession::ESTABLISHED) // active close
		{
			_current_state = tcpsession::FIN_WAIT_1;
			g_logger.printf("move to state FIN_WAIT_1\n");
			_expected_last_ack_seq_from_peer = pkt->get_seq() + pkt->get_tcp_content_len();
		}
		else if (_current_state == tcpsession::CLOSE_WAIT) // passive close
		{
			_current_state = tcpsession::LAST_ACK;
			g_logger.printf("move to state LAST_ACK\n");
			_expected_last_ack_seq_from_peer = pkt->get_seq() + pkt->get_tcp_content_len();
		}
		_last_seq_beyond_fin_at_localhost_side = pkt->get_seq() + pkt->get_tcp_content_len();
	}

	last_recorded_snd_time = jiffies;

	return count;
}

void tcpsession::got_a_packet(const ip_pkt* pkt)
{
	uint64_t jiffies = g_timer.get_jiffies();
	last_recorded_recv_time = jiffies;

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

void tcpsession::create_an_ack_without_payload()
{
	struct tcphdr* tcphdr;
	char buff[40];
	memcpy(buff, _ack_template, sizeof(buff));

	tcphdr = (struct tcphdr*)(buff + 20);
	tcphdr->seq = htons(_last_seq_beyond_fin_at_localhost_side);
	_ippkts_samples.push_back(buff);
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
	// this event rarely happens in real world.
	// TODO. add code to handle this case.
}

void tcpsession::syn_sent_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_syn_set())
	{
		if (pkt->is_ack_set())
		{
			_current_state = tcpsession::ESTABLISHED;
			g_logger.printf("move to state ESTABLISHED\n");
		}
		else
		{
			_current_state = tcpsession::SYN_RCVD; // rarely happens.
			g_logger.printf("move to state SYN_RCVD\n");
		}
	}
	refresh_status(pkt);
}

void tcpsession::established_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_fin_set())
	{
		_current_state = tcpsession::CLOSE_WAIT;
		g_logger.printf("move to state CLOSE_WAIT\n");
	}
	refresh_status(pkt);
}

void tcpsession::close_wait_state_handler(const ip_pkt* pkt)
{
	// this state will be transformed to LAST_ACK in the sending logic, refer to pls_send_these_packets().
	refresh_status(pkt);
}

void tcpsession::last_ack_state_handler(const ip_pkt* pkt)
{
	if (pkt->get_ack_seq() == _expected_last_ack_seq_from_peer)
	{
		_current_state = tcpsession::CLOSED;
		g_logger.printf("move to state CLOSED\n");
	}
	refresh_status(pkt);
}

void tcpsession::fin_wait_1_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_ack_set())
	{
		if (!pkt->is_fin_set() && pkt->get_ack_seq() == _expected_last_ack_seq_from_peer)
		{
			_current_state = tcpsession::FIN_WAIT_2;
			g_logger.printf("move to state FIN_WAIT_2\n");
		}
		else if(pkt->is_fin_set())
		{
			_current_state = tcpsession::CLOSING;
			g_logger.printf("move to state CLOSING\n");
		}
	}

	if (0 != pkt->get_tcp_content_len())
	{
		create_an_ack_without_payload();
		_sliding_window_right_boundary = _ippkts_samples.end();
	}
	refresh_status(pkt);
}

void tcpsession::fin_wait_2_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_fin_set())
	{
		_current_state = tcpsession::TIME_WAIT;
		g_logger.printf("move to state TIME_WAIT\n");
	}

	if (0 != pkt->get_tcp_content_len())
	{
		create_an_ack_without_payload();
		_sliding_window_right_boundary = _ippkts_samples.end();
	}
	refresh_status(pkt);
}

void tcpsession::closing_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_ack_set() && pkt->get_ack_seq() == _expected_last_ack_seq_from_peer)
	{
		_current_state = tcpsession::CLOSED;
		g_logger.printf("move to state CLOSED\n");
	}
	refresh_status(pkt);
}

void tcpsession::time_wait_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_ack_set() && pkt->get_ack_seq() == _expected_last_ack_seq_from_peer)
	{
		_current_state = tcpsession::CLOSED;
		g_logger.printf("move to state CLOSED\n");
	}
	refresh_status(pkt);
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

       // the second handshake.
	if (pkt->is_syn_set() && pkt->is_ack_set())
	{
		_expected_next_sequence_from_peer = pkt->get_seq() + 1;
		_latest_acked_sequence_by_peer = pkt->get_ack_seq();
		_advertised_window_size = pkt->get_win_size();
	}
	else
	{
		uint32_t next_sequence_from_peer = pkt->get_seq() + pkt->get_tcp_content_len();
		if (seq == _expected_next_sequence_from_peer)
		{
			_expected_next_sequence_from_peer = next_sequence_from_peer;
		}
		else
		{
			// drop outdated packets.
			if(seq_before(next_sequence_from_peer, _expected_next_sequence_from_peer))
			{
				return;
			}
		}
	}

	if (pkt->is_ack_set() && !pkt->is_syn_set())
	{
		ack_seq_tmp = pkt->get_ack_seq();
		// the peer acked new packe.
		if (seq_before(_latest_acked_sequence_by_peer, ack_seq_tmp))
		{
			_latest_acked_sequence_by_peer = pkt->get_ack_seq();
		}
	}

	// eliminate acked packets.
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
	int current_sliding_win_size = 0;
	int ippkt_count_walked_through = 0;
	for (ite = _sliding_window_left_boundary;
		 ite != _sliding_window_right_boundary;)
	{
		current_sliding_win_size += ite->get_tot_len();
		if (current_sliding_win_size > _advertised_window_size)
		{
			// make sure at least one IP packet are available to be sent.
			if(0 == ippkt_count_walked_through)
			{
				break;
			}
			else  // reduce the window size.
			{
			       // update the right boundary.
				_sliding_window_right_boundary = ite;
				break;
			}
		}
		ippkt_count_walked_through++;
		++ite;
	}
	std::cout << ippkt_count_walked_through << " packets are in the sliding window.\n";
	// // try to increase the sliding window size
	if (current_sliding_win_size < _advertised_window_size)
	{
		while (current_sliding_win_size < _advertised_window_size
				&& _sliding_window_right_boundary != _ippkts_samples.end())
		{
			++_sliding_window_right_boundary;
			current_sliding_win_size += ite->get_tot_len();
			if (current_sliding_win_size > _advertised_window_size)
			{
				break;
			}
		}
	}
	else // reduce the sliding window size
	{
		int how_much_bytes_should_be_reduced;
		how_much_bytes_should_be_reduced = current_sliding_win_size - _advertised_window_size;
		// TODO. sorry i have to ignore this logic at current cos I'm so tried to code and it gonna be still okay
		// without reducing sliding window size.
	}
}
