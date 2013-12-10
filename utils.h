/*********************************************
 * utils.cpp
 * Author: kamuszhou www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

/**
 *@param pcap The pcap handler.
 *@param frame The package captured on level 2.
 *@return The start address of IP package.
 */
const unsigned char* strip_l2head(pcap_t *pcap, const unsigned char *frame);

/* ip_pack is the starting address of a IP package. */
#define ip_package_parser(ip_pack)    struct iphdr *iphdr;   \
		struct tcphdr *tcphdr;                                     \
		unsigned char *tcp_content;                               \
		int iphdr_len, ip_content_len, tcphdr_len, tcp_content_len;     \
		\
		iphdr = (struct iphdr*)(ip_pack);                                   \
		iphdr_len = IP_HDR_LEN(iphdr);                                          \
		ip_content_len = ntohs(iphdr->tot_len) - IP_HDR_LEN(iphdr);                    \
		tcphdr = (struct tcphdr*)((unsigned char*)iphdr + IP_HDR_LEN(iphdr));   \
		tcphdr_len = TCP_HDR_LEN(tcphdr);                                                            \
		tcp_content = (unsigned char*)tcphdr + TCP_HDR_LEN(tcphdr);                 \
		tcp_content_len = ip_content_len -  TCP_HDR_LEN(tcphdr);                       \
		tcp_content_len = tcp_content_len; tcphdr_len = tcphdr_len;     \
		iphdr_len = iphdr_len; tcp_content = tcp_content;       /* suppress the warning: set but not used */

#define ip_package_clone(ip_pack)  ({   \
		int  tot_len;                                        \
		char  *pack_clone;                     \
		tot_len = ntohs(iphdr->tot_len);    \
		pack_clone = malloc(tot_len);        \
		memcpy(pack_clone, ip_pack, tot_len);    \
		pack_clone;   })

#define mk_sess_key(ip, port) ({    \
		uint64_t   k;                            \
		uint32_t *ptr;                       \
		ptr = (uint32_t*)&k;             \
		*ptr++ = ip;                           \
		*ptr = port;                            \
		k;   })
