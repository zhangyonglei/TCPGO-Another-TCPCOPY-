/*********************************************
 * ippkg.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: 12 Dec, 2013
 ********************************************/

#ifndef _IPPKG_H_
#define _IPPKG_H_

#include "misc.h"

class ip_pkt {
public:
	ip_pkt();
	ip_pkt(const ip_pkt& ip_pkg);
	ip_pkt(const unsigned char* ip_pkt);

	/**
	 * copy the ip packet pointed by parameter ip_pkt.
	 * the function's behavior is undefined if called with a invalid ip packet address.
	 * @ip_pkt point to the starting address of valid IP packet.
	 */
	void cp(const unsigned char* ip_pkt);

	const ip_pkt& operator=(const ip_pkt& ip_pkt);

	bool operator<(const ip_pkt& challenger)const;

	/**
	 * Parse the IP packet data and set the member fields appropriately.
	 */
	void warm_up();

	virtual ~ip_pkt();

private:
	unsigned char *_pkt;   ///< the starting address of the IP packet.

	int  _tot_len;               ///< the IP packet total length.
	struct iphdr *_iphdr;        ///< pointer to the ip header
	const unsigned char *_ip_content;  ///< pointer to the ip content excluding the ip header.
	int  _ihl;                   ///< the IP header length.

	struct tcphdr *_tcphdr;      ///< pointer to the tcp header.
	const unsigned char *_tcp_content; ///< pointer to the tcp content.

	unsigned int _seq;         ///< tcp's sequence number. In host byte order.
	unsigned int _ack_seq;     ///< tcp's acknoledgement sequence. In host byte order.

	bool _ack_flag;              ///< if the ack is set or not.
	bool _rst_flag;              ///< reset flag.
	bool _syn_flag;              ///< self-explanatory.
	bool _fin_flag;              ///< no explanation.
};

#endif /* _IPPKG_H_ */
