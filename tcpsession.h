/*********************************************
 * tcpsession.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#ifndef _TCPSESSION_H_
#define _TCPSESSION_H_

#include <list>
#include "misc.h"
#include "postoffice.h"
#include "ip_pkt.h"

class tcpsession : public postoffice_callback_interface
{
public:
	/**
	 *@param port in network byte order.
	 */
	tcpsession(uint32_t ip, uint16_t port);
	virtual ~tcpsession();

	/**
	 * add a ip packet to this session as a sample.
	 */
	void append_ip_sample(const char* ippkt);

	/**
	 * This function will also remove ack packets without playload.
	 * @return return 0 if the integrity check is passed, otherwise return non-zero error code.
	 */
	int check_samples_integrity();

public:
	/// refer to the interface postoffice_callback_interface for details.
	virtual int pls_send_these_packets(std::vector<const ip_pkt*>& pkts);
	virtual void got_a_packet(const ip_pkt *pkt);

private:
	/// 11 member functions for their respective tcp state.
	void closed_state_handler(const ip_pkt *pkt);
	void listen_state_handler(const ip_pkt *pkt);
	void syn_rcvd_state_handler(const ip_pkt *pkt);
	void syn_sent_state_handler(const ip_pkt *pkt);
	void established_state_handler(const ip_pkt *pkt);
	void close_wait_state_handler(const ip_pkt *pkt);
	void last_ack_state_handler(const ip_pkt *pkt);
	void fin_wait_1_state_handler(const ip_pkt *pkt);
	void fin_wait_2_state_handler(const ip_pkt *pkt);
	void closing_state_handler(const ip_pkt *pkt);
	void time_wait_state_handler(const ip_pkt *pkt);

private:
	std::list<ip_pkt>  _ippkts_samples;    ///< The ip packages which will be used to emulate the pseudo-client.

	// reserved for the possible future version if i can still play with this stuff.
//	std::list<ip_pkt>  _ippkts_received;   // The ip packages received from the server will be saved here.

	/// 11 standard TCP state.
	enum state_machine{CLOSED, LISTEN, SYN_RCVD, SYN_SENT, ESTABLISHED, CLOSE_WAIT, LAST_ACK, FIN_WAIT_1,
			FIN_WAIT_2, CLOSING, TIME_WAIT};
	state_machine _current_state;
	uint32_t _expected_next_sequence_from_peer;
	uint32_t _latest_acked_sequence_by_peer;

	uint32_t _client_src_ip_num;
	std::string _client_src_ip_str;
	uint16_t _client_src_port;      ///< in host byte order
};

#endif /* _TCPSESSION_H_ */
