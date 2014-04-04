/*********************************************
 * utils.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include "misc.h"

/**
 * Memory Block.
 */
typedef std::vector<char> MemBlock;

/**
 * the following macro is inspired by the book "Linux Kernel Development 3rd Edition" Page 214.
 * It gets around the problem of number wraparound.
 */
#define seq_before(left, right)  ((int)((uint32_t)(left - right)) < 0)
#define seq_before_eq(left, right)  ((int)((uint32_t)(left - right)) <= 0)
#define seq_after(left, right)  ((int)((uint32_t)(left - right)) > 0)
#define seq_after_eq(left, right)  ((int)((uint32_t)(left - right)) >= 0)

/**
 * calculate the IPv4 header checksum.
 * note the caller no longer has to set IP Header Checksum((char*)iphdr[10,11]) to zero.
 */
uint16_t compute_ip_checksum(const struct iphdr *iphdr);

/**
 * calculate the checksum of tcp over IPv4.
 * note the caller no longer has to set TCP Header Checksum((char*)tcphdr[16, 17] to zero.
 */
uint16_t compute_tcp_checksum(const struct iphdr *iphdr, const struct tcphdr *tcphdr);

/**
 *@param pcap The pcap handler.
 *@param frame The package captured on level 2.
 *@return The start address of IP package.
 */
const char* strip_l2head(pcap_t *pcap, const char *frame);

/**
 * detect the level2 header length in a ungraceful way. 
 */
int detect_l2head_len(const char *frame);

unsigned short generate_the_port(unsigned short ori_src_port);

/* ip_pack is the starting address of a IP package. */
#define ip_packet_parser(ip_pack)        \
        const struct iphdr *iphdr;   \
		const struct tcphdr *tcphdr;                                     \
		const char *tcp_payload;                               \
		int32_t ip_tot_len, iphdr_len, ip_payload_len, tcphdr_len, tcp_payload_len;     \
		\
		iphdr = (const struct iphdr*)(ip_pack);                                   \
		ip_tot_len = ntohs(iphdr->tot_len);                                 \
		iphdr_len = iphdr->ihl << 2;                                          \
		ip_payload_len = ip_tot_len - iphdr_len;                \
		tcphdr = (const struct tcphdr*)((const char*)iphdr + iphdr_len);   \
		tcphdr_len = tcphdr->doff << 2;                                                            \
		tcp_payload = (const char*)tcphdr + tcphdr_len;                \
		tcp_payload_len = ip_payload_len - tcphdr_len;                       \
		tcp_payload_len = tcp_payload_len; tcphdr_len = tcphdr_len;     \
		ip_tot_len = ip_tot_len; iphdr_len = iphdr_len; tcp_payload = tcp_payload;/* suppress the warning: set but not used */

#define ip_packet_clone(ip_pack)  ({   \
		int  tot_len;                                        \
		char  *pack_clone;                     \
		struct iphdr *iphdr;                  \
		iphdr = (struct iphdr*)ip_pack;       \
		tot_len = ntohs(iphdr->tot_len);    \
		pack_clone = new int8_t[tot_len];        \
		memcpy(pack_clone, ip_pack, tot_len);    \
		pack_clone;   })

/*
 * combine the ipv4 address and the port as a session key.
 * The caller should take responsibility of byte order consistency issue carefully.
 */
#define make_sess_key(ip, port) ({    \
		uint64_t   k;                            \
		uint32_t *ptr;                       \
		ptr = (uint32_t*)&k;             \
		*ptr++ = ip;                           \
		*ptr = port;                            \
		k;   })

/**
 * @param dir The base directory that will be searched.
 * @param regexp the regular expression that will be applied to the file name.
 * @param files The OUT parameter that will hold all the file paths on return.
 * @return 0 on success, non-zero on failure.
 */
int find_files(const std::string& dir, const std::string& regexp, std::list<std::string>& files);

uint32_t next_avail_ip(uint32_t orig_ip);

#endif
