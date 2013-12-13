/*********************************************
 * utils.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include "misc.h"

/**
 * the following macro is inspired by the book "Linux Kernel Development 3rd Edition" Page 214.
 * It gets around the problem of number wraparound.
 */
#define seq_before(left, right) ((int32_t)(left) - (int32_t)(right) < 0)
#define seq_after(left, right) ((int32_t)(left) - (int32_t)(right) > 0)

/**
 * calculate the IPv4 header checksum.
 */
unsigned short calc_ip_checksum(const unsigned short iphdr[], int32_t len);

/**
 *@param pcap The pcap handler.
 *@param frame The package captured on level 2.
 *@return The start address of IP package.
 */
const uint8_t* strip_l2head(pcap_t *pcap, const uint8_t *frame);

/**
 * detect the level2 header length in a ungraceful way. 
 */
int32_t detect_l2head_len(const uint8_t *frame);

/* ip_pack is the starting address of a IP package. */
#define ip_packet_parser(ip_pack)        \
        struct iphdr *iphdr;   \
		struct tcphdr *tcphdr;                                     \
		uint8_t *tcp_content;                               \
		int32_t ip_tot_len, iphdr_len, ip_content_len, tcphdr_len, tcp_content_len;     \
		\
		iphdr = (struct iphdr*)(ip_pack);                                   \
		ip_tot_len = ntohs(iphdr->tot_len);                                 \
		iphdr_len = iphdr->ihl << 2;                                          \
		ip_content_len = ip_tot_len - iphdr_len;                \
		tcphdr = (struct tcphdr*)((uint8_t*)iphdr + iphdr_len);   \
		tcphdr_len = tcphdr->doff << 2;                                                            \
		tcp_content = (uint8_t*)tcphdr + tcphdr_len;                \
		tcp_content_len = ip_content_len - tcphdr_len;                       \
		tcp_content_len = tcp_content_len; tcphdr_len = tcphdr_len;     \
		ip_tot_len = ip_tot_len; iphdr_len = iphdr_len; tcp_content = tcp_content;/* suppress the warning: set but not used */

#define ip_packet_clone(ip_pack)  ({   \
		int32_t  tot_len;                                        \
		int8_t  *pack_clone;                     \
		struct iphdr *iphdr;                  \
		iphdr = (struct iphdr*)ip_pack;       \
		tot_len = ntohs(iphdr->tot_len);    \
		pack_clone = new int8_t[tot_len];        \
		memcpy(pack_clone, ip_pack, tot_len);    \
		pack_clone;   })

#define mk_sess_key(ip, port) ({    \
		uint64_t   k;                            \
		uint32_t *ptr;                       \
		ptr = (uint32_t*)&k;             \
		*ptr++ = ip;                           \
		*ptr = port;                            \
		k;   })

#endif
