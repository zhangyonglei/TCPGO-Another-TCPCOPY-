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
	ip_pkt(const ip_pkt& pkt);
	ip_pkt(const char* pkt);

	/**
	 * copy the ip packet pointed by parameter ip_pkt.
	 * the function's behavior is undefined if called with a invalid ip packet address.
	 * @ip_pkt point to the starting address of valid IP packet.
	 */
	void cp(const char* pkt);

	void swap(ip_pkt& pkt);

	const ip_pkt& operator=(const ip_pkt& pkt);

	bool operator<(const ip_pkt& challenger)const;
	bool operator>(const ip_pkt& challenger)const;
	bool operator==(const ip_pkt& challenger)const;

	/**
	 * Parse the IP packet data and set the member fields appropriately.
	 */
	void warm_up();

	virtual ~ip_pkt();

public:
	const char* get_starting_addr()const
	{
		return _pkt;
	}
	
	int32_t get_tot_len()const
	{
		return _tot_len;
	}

	int32_t get_tcp_content_len()const
	{
		return _tcp_content_len;
	}

	uint32_t get_seq()const
	{
		return _seq;
	}

	uint32_t get_ack_seq()const
	{
		return _ack_seq;
	}

	bool is_ack_set()const
	{
		return _ack_flag;
	}

	bool is_rst_set()const
	{
		return _rst_flag;
	}

	bool is_syn_set()const
	{
		return _syn_flag;
	}

	bool is_fin_set()const
	{
		return _fin_flag;
	}

private:
	const char *_pkt;   ///< the starting address of the IP packet.

	int32_t  _tot_len;               ///< the IP packet total length.
	struct iphdr *_iphdr;        ///< pointer to the ip header
	const uint8_t *_ip_content;  ///< pointer to the ip content excluding the ip header.
	int32_t  _ihl;                   ///< the IP header length.

	struct tcphdr *_tcphdr;      ///< pointer to the tcp header.
	const uint8_t *_tcp_content; ///< pointer to the tcp content.
	int32_t _tcp_content_len;        ///< the length of the tcp content AKA playload.

	uint32_t _seq;         ///< tcp's sequence number. In host byte order.
	uint32_t _ack_seq;     ///< tcp's acknoledgement sequence. In host byte order.

	bool _ack_flag;              ///< if the ack is set or not.
	bool _rst_flag;              ///< reset flag.
	bool _syn_flag;              ///< self-explanatory.
	bool _fin_flag;              ///< no explanation.

	std::string _client_ip_addr;
};

#endif /* _IPPKG_H_ */
