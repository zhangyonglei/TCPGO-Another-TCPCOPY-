/*********************************************
 * ippkg.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: 12 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _IPPKG_H_
#define _IPPKG_H_

#include "misc.h"

class ip_pkt {
public:
	ip_pkt();
	ip_pkt(const char* pkt);

	/**
	 * copy the ip packet pointed by parameter ip_pkt.
	 * the function's behavior is undefined if called with a invalid ip packet address.
	 * @ip_pkt point to the starting address of valid IP packet.
	 */
	void cp(const char* pkt);

	void swap(ip_pkt& pkt);

	bool operator<(const ip_pkt& challenger)const;
	bool operator>(const ip_pkt& challenger)const;
	bool operator==(const ip_pkt& challenger)const;

	virtual ~ip_pkt();

public:
	/**
	 * @param src_port in host byte order.
	 */
	void  modify_src_port(uint16_t src_port);

	/**
	 * return the previous tcp checksum, and new tcp checksum is calculated and set.
	 * note: in network byte order.
	 */
	uint16_t reset_tcp_checksum();

	/**
	 * return the previous ip checksum, and new tcp checksum is calculated and set.
	 * note: in network byte order.
	 */
	uint16_t reset_ip_checksum();
	
	const struct iphdr* get_iphdr()const
	{
		return _iphdr;
	}

	const struct tcphdr* get_tcphdr()const
	{
		return _tcphdr;
	}
	
	const char* get_starting_addr()const
	{
		return _pkt.get();
	}
	

	int get_tot_len()const
	{
		return _tot_len;
	}

	int get_iphdr_len()const
	{
		return _iphdr_len;
	}

	int get_tcphdr_len()const
	{
		return _tcphdr_len;
	}

	int get_tcp_payload_len()const
	{
		return _tcp_payload_len;
	}

	const char* get_tcp_payload()const
	{
		return _tcp_payload;
	}

	uint32_t get_seq()const
	{
		return _seq;
	}

	uint32_t get_ack_seq()const
	{
		return _ack_seq;
	}

	uint16_t get_win_size()const
	{
		return _win_size;
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

	const std::string get_src_addr()const
	{
		return _src_addr;
	}

	const std::string get_dst_addr()const
	{
		return _dst_addr;
	}

	// in host byte order
	uint16_t get_src_port()const
	{
		return ntohs(_tcphdr->source);
	}

	// in host byte order
	uint16_t get_dst_port()const
	{
		return ntohs(_tcphdr->dest);
	}

	/**
	 * replace the original destination IP address and port with new
	 * dest IP addr and port. checksum will be re-calculated and properly
	 * stored.
	 * @param port the dst port in host byte order.
	 */
	void rebuild(const char* addr, unsigned short port, uint32_t expected_next_sequence_from_peer);

private:
	/**
	 * Parse the IP packet data and set the member fields appropriately.
	 */
	void warm_up();

private:
	// char *_pkt;   ///< the starting address of the IP packet.
	boost::shared_ptr<char> _pkt;

	int  _tot_len;               ///< the IP packet total length.
	struct iphdr *_iphdr;        ///< pointer to the ip header
	char *_ip_content;  ///< pointer to the ip content excluding the ip header.
	int  _iphdr_len;                   ///< the IP header length.

	struct tcphdr *_tcphdr;      ///< pointer to the tcp header.
	int  _tcphdr_len;             ///< the TCP header length.
	char *_tcp_payload; ///< pointer to the tcp payload.
	int _tcp_payload_len;        ///< the length of the tcp playload.

	uint32_t _seq;         ///< tcp's sequence number. In host byte order.
	uint32_t _ack_seq;     ///< tcp's acknoledgement sequence. In host byte order.
	uint16_t _win_size;    ///< advertised window size.

	bool _ack_flag;              ///< if the ack is set or not.
	bool _rst_flag;              ///< reset flag.
	bool _syn_flag;              ///< self-explanatory.
	bool _fin_flag;              ///< no explanation.

	/// the following variables are for debug's convenience.
	std::string _src_addr;
	std::string _dst_addr;
};

#endif /* _IPPKG_H_ */
