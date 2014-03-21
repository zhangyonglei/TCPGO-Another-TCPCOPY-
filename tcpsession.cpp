/*********************************************
 * tcpsession.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "tcpsession.h"
#include "utils.h"
#include "cute_logger.h"
#include "thetimer.h"
#include "session_manager.h"
#include "configuration.h"
#include "statistics_bureau.h"
#include "testsuite.h"

tcpsession::tcpsession(uint32_t ip, uint16_t port)
{
	struct in_addr inaddr;
	inaddr.s_addr = ip;

	_client_src_ip_num = ip;
	_client_src_ip_str = inet_ntoa(inaddr);
	_client_src_port = ntohs(port);
	_session_key = mk_sess_key(ip, port);

	_response_from_peer_time_out = g_configuration.get_response_from_peer_time_out();
	_have_to_send_data_within_this_timeperiod = g_configuration.get_have_to_send_data_within_this_timeperiod();
	_snd_speed_control = g_configuration.get_snd_speed_control();
	_wait_for_fin_from_peer_time_out = g_configuration.get_wait_for_fin_from_peer_time_out();
	_enable_active_close = g_configuration.get_enable_active_close();

	struct iphdr *iphdr = (struct iphdr*)_pure_ack_template;
	struct tcphdr *tcphdr = (struct tcphdr*)(_pure_ack_template + 20);
	memset(_pure_ack_template, 0, 40);
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
	tcphdr->ack = 1;
	tcphdr->doff = 5;
	tcphdr->window = htons(65535);
	tcphdr->check = 0;
	tcphdr->urg_ptr = 0;

	memcpy(_pure_rst_template, _pure_ack_template, sizeof(_pure_rst_template));
	tcphdr = (struct tcphdr*)(_pure_rst_template + 20);
	tcphdr->ack = 0;
	tcphdr->rst = 1;
}

tcpsession::~tcpsession()
{
}

void tcpsession::kill_me(cause_of_death cause)
{
	_dead = true;
	g_logger.printf("session: %s.%hu ended.\n", _client_src_ip_str.c_str(), _client_src_port);
	g_testsuite.report_sess_traffic(_client_src_ip_str, _client_src_port, _traffic_history, cause);

	if (PEER_TIME_OUT == cause || DORMANCY == cause)
	{
		_reset_the_peer = true;
	}
}

bool tcpsession::still_alive()
{
	return !_dead;
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

void tcpsession::inject_a_realtime_ippkt(const char* ippkt)
{
	std::list<ip_pkt>::iterator ite;
	ip_packet_parser(ippkt);
	if (tcp_payload_len == 0 && !tcphdr->fin && !tcphdr->syn)
	{
		return;
	}

	_ippkts_samples.push_back(ippkt);
	_ippkts_samples.sort();
	_sliding_window_left_boundary = _ippkts_samples.begin();
	if (_ippkts_samples.front().is_syn_set())
	{
		ite = _sliding_window_left_boundary;
		ite->mark_me_should_be_sent();
		++ite;
		_sliding_window_right_boundary = ite;
	}
}

int32_t tcpsession::check_samples_integrity()
{
	int32_t size_saved, size_now;
	int32_t i;   // for the convenience of debug.
	uint32_t seq;
	uint32_t expected_next_seq;
	int32_t tcp_payload_len;

	i = 0;
	std::list<ip_pkt>::iterator ite;
	// reomve the packet without tcp playload. e.g. the tcp keep-alive segments which are
	// used to elicit an ACK from the receiver.
	for(ite = _ippkts_samples.begin(); ite != _ippkts_samples.end();)
	{
		int tot_len = ite->get_tot_len();
		int iphdr_len = ite->get_iphdr_len();
		int tcphdr_len = ite->get_tcphdr_len();
		bool fin_set = ite->is_fin_set();
		bool ack_set = ite->is_ack_set();
		bool syn_set = ite->is_syn_set();
		bool rst_set = ite->is_rst_set();

		tcp_payload_len = ite->get_tcp_payload_len();

		// remove usefuless samples
		if (0 == tcp_payload_len && !fin_set && !syn_set ) // && !rst_set)  // rst is not allowed any more.
		{
			_ippkts_samples.erase(ite++);
		}
		// remove  corrupted sample, this case occurs rarely.
		else if (tot_len != iphdr_len + tcphdr_len + tcp_payload_len)
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
//			if (i + 1 == size_now && ite->is_rst_set() && expected_next_seq + 1 == seq) // Deprecated !!!
//			{
//				_ippkts_samples.erase(++ite, _ippkts_samples.end());
//				return 0;
//			}

			goto _err;
		}
		tcp_payload_len = ite->get_tcp_payload_len();
		if (tcp_payload_len > 0)
		{
			expected_next_seq += tcp_payload_len;
		}

		if(ite->is_fin_set())
		{
			_ippkts_samples.erase(++ite, _ippkts_samples.end());
			return 0;
		}
	}

_err:
	return 1;
}

void tcpsession::get_ready()
{
	std::list<ip_pkt>::iterator ite, tmp_ite;

	_dead = false;
	_reset_the_peer = false;
	_current_state = tcpsession::CLOSED;
	_expected_next_sequence_from_peer = 0;
	_latest_acked_sequence_by_peer = 0;
	_expected_last_ack_seq_from_peer = 0;
	_last_seq_beyond_fin_at_localhost_side = 0;
	_last_sent_byte_seq_beyond = 0;
	_advertised_window_size = 0;
	_sliding_window_left_boundary = _ippkts_samples.begin();
	if (!_ippkts_samples.empty())
	{
		tmp_ite = _sliding_window_left_boundary;
		++tmp_ite;
		_sliding_window_right_boundary = tmp_ite;
	}
	else
	{
		// empty
		_sliding_window_right_boundary = _ippkts_samples.end();
	}
	_last_recorded_recv_time = -1;
	_last_recorded_snd_time = g_timer.get_jiffies();

	for(ite = _ippkts_samples.begin(); ite != _ippkts_samples.end(); ++ite)
	{
		// add code to fiddle with every IP packets.
	}

	g_logger.printf("session %s.%hu is ready.\n", _client_src_ip_str.c_str(), _client_src_port);
}

int tcpsession::pls_send_these_packets(std::vector<ip_pkt*>& pkts)
{
	ip_pkt* pkt;
	int count;
	uint64_t jiffies;
	bool fin_has_been_sent;
	std::list<ip_pkt>::iterator ite;

	pkts.clear();

	if(!still_alive())
	{
		if (_reset_the_peer)  // send two RSTs in a row.
		{
			char buff[40];
			_reset_the_peer = false;
			memcpy(buff, _pure_rst_template, sizeof(buff));
			struct tcphdr* tcphdr = (struct tcphdr*)(buff + 20);
			tcphdr->seq = htonl(_latest_acked_sequence_by_peer);
			_pure_rst_pkt.cp(buff);
			_pure_rst_pkt.rebuild(g_configuration.get_dst_addr().c_str(),
						g_configuration.get_dst_port(), _expected_next_sequence_from_peer);
			pkts.push_back(&_pure_rst_pkt);

			tcphdr->seq = htonl(_last_sent_byte_seq_beyond);
			_pure_rst_pkt.cp(buff);
			_pure_rst_pkt.rebuild(g_configuration.get_dst_addr().c_str(),
						g_configuration.get_dst_port(), _expected_next_sequence_from_peer);
			pkts.push_back(&_pure_rst_pkt);

			return 2;  // send a RST.
		}
		else
		{
			return -1;
		}
	}

	jiffies = g_timer.get_jiffies();

	// don't send too quickly.
	if (jiffies - _last_recorded_snd_time <= _snd_speed_control)
	{
		if (_sliding_window_left_boundary == _sliding_window_right_boundary)
			return 0;

		pkt = &(*_sliding_window_left_boundary);

		bool need_send = false;
		ite = _sliding_window_right_boundary;
		--ite;
		do
		{
			if (ite->should_send_me())
			{
				need_send = true;
				break;
			}
			--ite;
		}while(ite != _sliding_window_left_boundary);

		// packets in sliding window have all be sent. Have to wait for the send speed control timer
		// to expire to get the chance of re-transmit.
		if (!need_send)
		{
			return 0;
		}
		else
		{
			// fall through...............
		}
	}
	else  // send speed control timer has expired.
	{
		for (ite = _sliding_window_left_boundary; ite != _sliding_window_right_boundary; ++ite)
		{
			ite->mark_me_should_be_sent();
		}
	}

	fin_has_been_sent = false;
	for(std::list<ip_pkt>::iterator ite = _sliding_window_left_boundary;
			ite != _sliding_window_right_boundary;
			++ite)
	{

		bool is_syn_set, is_ack_set, is_fin_set, is_rst_set;
		int tcp_payload_len;

		pkt = &(*ite);

		is_syn_set = pkt->is_syn_set();
		is_ack_set = pkt->is_ack_set();
		is_fin_set = pkt->is_fin_set();
		is_rst_set = pkt->is_rst_set();

		tcp_payload_len = pkt->get_tcp_payload_len();

		if (is_fin_set)  // FIN packet
		{
			if (_enable_active_close)
			{
				fin_has_been_sent = true;
			}
			else   // passive close
			{
				// halt my FIN and wait for FIN from the peer.
				if (_current_state != ESTABLISHED)
				{
					fin_has_been_sent = true;
				}
				else
				{
					break;
				}
			}
		}

		// if this is a pure ack
		if (is_ack_set && 0 == tcp_payload_len && !is_syn_set && !is_rst_set && !is_fin_set )
		{
			int sent_count;
			sent_count = pkt->get_sent_counter();

			if (0 == sent_count)  // record the pure ack that will be send.
			{
				_traffic_history.push_back(*pkt);
			}
			else  // pure ack is one-shot only.
			{
				if (ite == _sliding_window_left_boundary)
				{
					++_sliding_window_left_boundary;
				}
				_ippkts_samples.erase(ite++);

				continue;
			}
		}

		pkt->mark_me_should_be_sent();
		pkt->rebuild(g_configuration.get_dst_addr().c_str(),
					g_configuration.get_dst_port(), _expected_next_sequence_from_peer);
		pkts.push_back(pkt);
		_last_sent_byte_seq_beyond = pkt->get_seq() + 1;
	} // end the loop of the sliding window

	count = pkts.size();

	if (0 != count && pkts[0]->is_syn_set())
	{
		_current_state = tcpsession::SYN_SENT;
		g_logger.printf("session: %s.%hu moves to state SYN_SENT from state CLOSED.\n",
				_client_src_ip_str.c_str(), _client_src_port);
		// I cannot remember why I write this line, it looks like unnecessary.
		// So i comment it out.
		// _last_recorded_recv_time = jiffies;
	}
	else
	{
		// timeout. No responses has received from peer for a long time.
		if (_last_recorded_recv_time != -1 && jiffies - _last_recorded_recv_time > _response_from_peer_time_out )
		{
			const char* ip_str;
			ip_str = _client_src_ip_str.c_str();
			g_logger.printf("session: %s.%hu time out. I commit a suicide.\n", ip_str, _client_src_port);
			g_statistics_bureau.inc_sess_cancelled_by_no_response_count();
			kill_me(PEER_TIME_OUT);
			return 0;
		}
	}

	if (0 != count && fin_has_been_sent)
	{
		const ip_pkt* pkt = pkts[count-1];
		if (_current_state == tcpsession::ESTABLISHED) // active close
		{
			_current_state = tcpsession::FIN_WAIT_1;
			g_logger.printf("session: %s.%hu moves to state FIN_WAIT_1 from state ESTABLISHED.\n",
					_client_src_ip_str.c_str(), _client_src_port);
			_expected_last_ack_seq_from_peer = pkt->get_seq() + pkt->get_tcp_payload_len();
		}
		else if (_current_state == tcpsession::CLOSE_WAIT) // passive close
		{
			_current_state = tcpsession::LAST_ACK;
			g_logger.printf("session: %s.%hu moves to state LAST_ACK from state CLOSE_WAIT.\n",
					_client_src_ip_str.c_str(), _client_src_port);
			_expected_last_ack_seq_from_peer = pkt->get_seq() + pkt->get_tcp_payload_len();
		}
		_last_seq_beyond_fin_at_localhost_side = pkt->get_seq() + pkt->get_tcp_payload_len();
	}

	if (count > 0 && _current_state == tcpsession::TIME_WAIT)
	{
		// Give only one chance for peer's FIN to be acked.
		g_logger.printf("session %s.%hu exits from state TIME_WAIT.\n", _client_src_ip_str.c_str(), _client_src_port);
		g_statistics_bureau.inc_sess_active_close_count();
		kill_me(ACTIVE_CLOSE);
	}

	if (count > 0)
	{
		_last_recorded_snd_time = jiffies;
	}
	else if (jiffies - _last_recorded_snd_time > _have_to_send_data_within_this_timeperiod)
	{
		g_statistics_bureau.inc_sess_dormancy_count();
		kill_me(DORMANCY);
		return 0;
	}

	return count;
}

void tcpsession::got_a_packet(const ip_pkt* pkt)
{
	uint64_t jiffies = g_timer.get_jiffies();
	_last_recorded_recv_time = jiffies;

	if (!still_alive())
		return;

	if (pkt->is_rst_set())
	{
		g_logger.printf("session: %s.%hu reset kills me.\n", _client_src_ip_str.c_str(), _client_src_port);
		g_statistics_bureau.inc_sess_killed_by_reset_count();
		_traffic_history.push_back(*pkt);
		kill_me(RESET);
		return;
	}

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

void tcpsession::create_an_ack_without_payload(uint32_t seq)
{
	struct tcphdr* tcphdr;
	char buff[40];
	memcpy(buff, _pure_ack_template, sizeof(buff));

	if (!_ippkts_samples.empty())
		return;

	tcphdr = (struct tcphdr*)(buff + 20);
	tcphdr->seq = htons(seq);
	tcphdr->ack = 1;
	_ippkts_samples.push_back(buff);
	assert(!tcphdr->fin);
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
			g_logger.printf("session %s.%hu moves to state ESTABLISHED from state SYN_SENT.\n",
					_client_src_ip_str.c_str(), _client_src_port);
		}
		else
		{
			_current_state = tcpsession::SYN_RCVD; // rarely happens.
			g_logger.printf("session %s.%hu moves to state SYN_RCVD from state SYN_SENT.\n",
					_client_src_ip_str.c_str(), _client_src_port);
		}
	}
	refresh_status(pkt);
}

void tcpsession::established_state_handler(const ip_pkt* pkt)
{
	if (pkt->is_fin_set())
	{
		_current_state = tcpsession::CLOSE_WAIT;
		g_logger.printf("session %s.%hu moves to state CLOSE_WAIT from state ESTABLISHED.\n",
				_client_src_ip_str.c_str(), _client_src_port);
	}
	refresh_status(pkt);
}

void tcpsession::close_wait_state_handler(const ip_pkt* pkt)
{
	// this state will be transformed to LAST_ACK in the sending logic, refer to pls_send_these_packets().
	refresh_status(pkt);

	if (0 != pkt->get_tcp_payload() && _ippkts_samples.empty() )
	{
		create_an_ack_without_payload(_latest_acked_sequence_by_peer);
		_sliding_window_left_boundary = _ippkts_samples.begin();
		_sliding_window_right_boundary = _ippkts_samples.end();
	}
}

void tcpsession::last_ack_state_handler(const ip_pkt* pkt)
{
	uint32_t ack_seq;

	refresh_status(pkt);

	ack_seq = pkt->get_ack_seq();
	if (pkt->is_ack_set() && seq_before_eq(_expected_last_ack_seq_from_peer, ack_seq))
	{
		_current_state = tcpsession::CLOSED;
		g_logger.printf("session %s.%hu moves to state CLOSED from state LAST_ACK.\n",
				_client_src_ip_str.c_str(), _client_src_port);
		g_statistics_bureau.inc_sess_passive_close_count();
		_traffic_history.push_back(*pkt);
		kill_me(PASSIVE_CLOSE);
		return;
	}
}

void tcpsession::fin_wait_1_state_handler(const ip_pkt* pkt)
{
	bool my_fin_has_been_acked;
	uint32_t ack_seq;

	ack_seq = pkt->get_ack_seq();
	if (pkt->is_ack_set() && seq_before_eq(_expected_last_ack_seq_from_peer, ack_seq))
	{
		my_fin_has_been_acked = true;
		_my_fin_acked_time = g_timer.get_jiffies();
	}
	else
	{
		my_fin_has_been_acked = false;
	}

	if (my_fin_has_been_acked && !pkt->is_fin_set() )
	{
		_current_state = tcpsession::FIN_WAIT_2;
		g_logger.printf("session: %s.%hu moves to state FIN_WAIT_2 from state FIN_WAIT_1.\n",
				_client_src_ip_str.c_str(), _client_src_port);
	}
	else if (my_fin_has_been_acked && pkt->is_fin_set())
	{
		_current_state = tcpsession::TIME_WAIT;
		g_logger.printf("session: %s.%hu moves to state TIME_WAIT from state FIN_WAIT_1.\n",
				_client_src_ip_str.c_str(), _client_src_port);
	}
	else if(pkt->is_fin_set())
	{
		_current_state = tcpsession::CLOSING;
		g_logger.printf("session: %s.%hu moves to state CLOSING from state FIN_WAIT_1.\n",
				_client_src_ip_str.c_str(), _client_src_port);
	}

	refresh_status(pkt);
	if (my_fin_has_been_acked)
	{
		create_an_ack_without_payload(_last_seq_beyond_fin_at_localhost_side);
		_sliding_window_left_boundary = _ippkts_samples.begin();
		_sliding_window_right_boundary = _ippkts_samples.end();
	}
}

void tcpsession::fin_wait_2_state_handler(const ip_pkt* pkt)
{
	uint64_t now;
	now = g_timer.get_jiffies();
	// my impatience is limited. My FIN has been sent for a long time without your FIN as a response.
	// I will commit a suicide.
	if (now - _last_recorded_recv_time > _wait_for_fin_from_peer_time_out)
	{
		g_logger.printf("session: %s.%hu No patience for your FIN. I commit a suicide.\n",
				_client_src_ip_str.c_str(), _client_src_port);
		g_statistics_bureau.inc_sess_active_close_timeout_count();
		_traffic_history.push_back(*pkt);
		kill_me(NO_FIN_FROM_PEER);
		return;
	}

	if (pkt->is_fin_set())
	{
		_current_state = tcpsession::TIME_WAIT;
		g_logger.printf("session: %s.%hu moves to state TIME_WAIT from state FIN_WAIT_2.\n",
				_client_src_ip_str.c_str(), _client_src_port);
	}

	refresh_status(pkt);

	if (pkt->get_tcp_payload_len())
	{
		create_an_ack_without_payload(_last_seq_beyond_fin_at_localhost_side);
		_sliding_window_left_boundary = _ippkts_samples.begin();
		_sliding_window_right_boundary = _ippkts_samples.end();
	}
}

void tcpsession::closing_state_handler(const ip_pkt* pkt)
{
	uint32_t ack_seq;

	ack_seq = pkt->get_ack_seq();
	if (pkt->is_ack_set() && seq_before_eq(_expected_last_ack_seq_from_peer, ack_seq))
	{
		_current_state = tcpsession::TIME_WAIT;
		g_logger.printf("session: %s.%hu moves to state TIME_WAIT from state CLOSING.\n",
				_client_src_ip_str.c_str(), _client_src_port);
		_my_fin_acked_time = g_timer.get_jiffies();
	}
	refresh_status(pkt);

	create_an_ack_without_payload(_last_seq_beyond_fin_at_localhost_side);
	_sliding_window_left_boundary = _ippkts_samples.begin();
	_sliding_window_right_boundary = _ippkts_samples.end();
}

void tcpsession::time_wait_state_handler(const ip_pkt* pkt)
{
	refresh_status(pkt);

	create_an_ack_without_payload(_last_seq_beyond_fin_at_localhost_side);
	_sliding_window_left_boundary = _ippkts_samples.begin();
	_sliding_window_right_boundary = _ippkts_samples.end();
}

std::list<ip_pkt>::iterator tcpsession::check_ippkts_continuity(std::list<ip_pkt>::iterator begin, std::list<ip_pkt>::iterator end)
{
	uint32_t seq, expected_next_seq;
	int tcp_payload_len;
	std::list<ip_pkt>::iterator ite, ite_pre;

	ite = begin;
	ite_pre = ite;
	if (begin == end)
	{
		return ite_pre;
	}

	seq = ite->get_seq();
	expected_next_seq = seq + ite->get_tcp_payload_len();
	if (ite->is_syn_set())
	{
		expected_next_seq++;
	}
	++ite;

	for (; ite != end; ++ite)
	{
		seq = ite->get_seq();
		if(expected_next_seq != seq)
		{
			break;
		}
		tcp_payload_len = ite->get_tcp_payload_len();
		assert(tcp_payload_len >= 0);
		expected_next_seq += tcp_payload_len;
		ite_pre = ite;
	}

	return ite_pre;
}

void tcpsession::refresh_status(const ip_pkt* pkt)
{
	uint32_t seq;
	uint32_t ack_seq;
	uint32_t ack_seq_tmp;
	uint16_t win_size_saved;
	int distance;

	std::list<ip_pkt>::iterator ite;

	seq = pkt->get_seq();
	ack_seq = pkt->get_ack_seq();

	// the following logic compute some sequence number
	if (pkt->is_syn_set() && pkt->is_ack_set())   // the second handshake.
	{
		_expected_next_sequence_from_peer = pkt->get_seq() + 1;
		_latest_acked_sequence_by_peer = pkt->get_ack_seq();
		_advertised_window_size = pkt->get_win_size();
	}
	else
	{
		uint32_t next_sequence_from_peer = pkt->get_seq() + pkt->get_tcp_payload_len();
		if (seq == _expected_next_sequence_from_peer)
		{
			_expected_next_sequence_from_peer = next_sequence_from_peer;
			if (pkt->is_fin_set())
			{
				_expected_next_sequence_from_peer++;
			}
		}
		else
		{
			// drop the outdated incoming packets.
			if(seq_before(next_sequence_from_peer, _expected_next_sequence_from_peer))
			{
				return;
			}
			else  // lost packets or packets are out of order. just let it go.
			{
				// NOTICE. Packets losing or packets out of order will be found in the traffic history.
			}
		}
	}

	if (pkt->is_ack_set() && !pkt->is_syn_set())
	{
		// the peer acked new packet.
		if (seq_before(_latest_acked_sequence_by_peer, ack_seq))
		{
			_latest_acked_sequence_by_peer = ack_seq;
		}
	}

	// eliminate acked packets.
	for (ite = _sliding_window_left_boundary;
			ite != _sliding_window_right_boundary;)
	{
		if (seq_before(ite->get_seq(), _latest_acked_sequence_by_peer))
		{
			_traffic_history.push_back(*ite);  // save the outgoing packets that have been sent
			_ippkts_samples.erase(ite++);
			_sliding_window_left_boundary = ite;
		}
		else
		{
			break;
		}
	}
	_traffic_history.push_back(*pkt);  // save the incoming traffic.

	win_size_saved = _advertised_window_size;
	_advertised_window_size = pkt->get_win_size();

	// should the sliding window be reduced.
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
			else  // reduce the window size
			{
				// update the right boundary.
				_sliding_window_right_boundary = ite;
				break;
			}
		}
		ippkt_count_walked_through++;
		++ite;
	}
	distance = std::distance(_sliding_window_left_boundary, _sliding_window_right_boundary);
	g_logger.printf("%d packet(s) is(are) in the sliding window.\n", distance);

	// should the sliding window size be increased.
	if (current_sliding_win_size < _advertised_window_size)
	{
		// try to determine how far it can go to increase sliding window.
		std::list<ip_pkt>::iterator right_gap, ite_left;
		if (_sliding_window_right_boundary != _ippkts_samples.end()) // got the chance to expand the window
		{
			if (_sliding_window_right_boundary != _sliding_window_right_boundary)
			{
				ite_left = _sliding_window_right_boundary;
				--ite_left;
				right_gap = check_ippkts_continuity(ite_left, _ippkts_samples.end());
			}
			else
			{
				ite_left = _sliding_window_left_boundary;
				right_gap = check_ippkts_continuity(ite_left, _ippkts_samples.end());
			}

			assert(right_gap != _ippkts_samples.end());
			// increase it because of closed interval (excluding right boundary)
			++right_gap;
		}
		else // cannot expand the window size by any means.
		{
			right_gap = _sliding_window_right_boundary;
		}

		while (current_sliding_win_size < _advertised_window_size
				&& _sliding_window_right_boundary != right_gap)
		{
			++_sliding_window_right_boundary;
			current_sliding_win_size += ite->get_tot_len();
			if (current_sliding_win_size > _advertised_window_size)
			{
				break;
			}
		}

		distance = std::distance(_sliding_window_left_boundary, _sliding_window_right_boundary);

		g_logger.printf("sliding window has been expanded to %d packets.\n", distance);
		g_logger.printf("%d packet(s) is(are) in the _ippkts_samples.\n", _ippkts_samples.size());
	}

	// make a mark that all packets in the sliding window should be send.
	for (ite = _sliding_window_left_boundary; ite != _sliding_window_right_boundary; ++ite)
	{
		ite->mark_me_should_be_sent();
	}
}
