/*********************************************
 * utils.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#include "misc.h"
#include "utils.h"
#include "configuration.h"

struct eth_hdr
{
	uint8_t _dest[6];
	uint8_t _src[6];
	uint16_t _type;
};

uint16_t compute_ip_checksum(const struct iphdr* iphdr)
{
	int hdrlen;
	uint32_t sum = 0, sum_saved;
	const char *mem = (const char*) iphdr;

	assert(NULL != iphdr);
	sum_saved = iphdr->check;
	*(unsigned short*) (&iphdr->check) = 0;

	hdrlen = (iphdr->ihl << 2);

	while (hdrlen > 1)
	{
		sum += *(uint16_t*) mem;
		mem += 2;
		if (sum & 0x80000000) /* if high order bit set, fold */
			sum = (sum & 0xFFFF) + (sum >> 16);
		hdrlen -= 2;
	}

	if (hdrlen) /* take care of left over byte */
	{ //notice: i think there lives a bug 'cos the btye after mem is possibly not zero.
		sum += (uint16_t) * (uint8_t *) mem;
		std::cerr << "notice " << __FILE__ << " " << __LINE__ << std::endl;
	}

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	*(unsigned short*) &(iphdr->check) = sum_saved;
	return ~sum;
}

uint16_t compute_tcp_checksum(const struct iphdr *iphdr,
		const struct tcphdr *tcphdr)
{
	int i, need_pad, loop_size;
	int tot_tcpsgmt_len;
	int pseudo_hdr_len;
	int tot_ipsegmt_len;
	uint16_t word16;
	uint16_t buff[12];
	uint16_t* ptr2buff;
	uint32_t sum, sum_saved;
	const uint16_t* tcpmem;

	const uint8_t* src_addr;
	const uint8_t* dst_addr;

	tcpmem = (uint16_t*) tcphdr;
	sum_saved = tcphdr->check;
	*(unsigned short*) (&tcphdr->check) = 0;

	tot_ipsegmt_len = ntohs(iphdr->tot_len);
	tot_tcpsgmt_len = tot_ipsegmt_len - ((iphdr->ihl) << 2);
	//assert(tot_tcpsgmt_len>=0);

	//initialize sum to zero
	sum = 0;

	memcpy(buff, &iphdr->saddr, 4);
	memcpy(buff + 2, &iphdr->daddr, 4);
	buff[4] = htons((unsigned short) (iphdr->protocol));
	buff[5] = htons((unsigned short) tot_tcpsgmt_len);
	pseudo_hdr_len = sizeof(buff) / sizeof(buff[0]);
	ptr2buff = buff;
	while (pseudo_hdr_len > 1)
	{
		sum += *(uint16_t*) ptr2buff;
		ptr2buff++;
		pseudo_hdr_len -= 2;
	}

	while (tot_tcpsgmt_len > 1)
	{
		sum += *(uint16_t*) tcpmem;
		tcpmem++;
		tot_tcpsgmt_len -= 2;
	}

	if (tot_tcpsgmt_len) /* take care of left over byte */
	{ //notice: i think there lives a bug 'cos the btye after mem is possibly not zero.
		sum += (uint16_t) * (uint8_t *) tcpmem;

	}

	// keep only the last 16 bits of the 32 bit calculated sum and add the carries
	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	sum = ~sum;

	*(unsigned short*) &(tcphdr->check) = sum_saved;

	return ((unsigned short) sum);
}

const char* strip_l2head(pcap_t *pcap, const char *frame)
{
	int32_t l2_len;
	struct eth_hdr *ethhdr;

	l2_len = -1;
	switch (pcap_datalink(pcap))
	{
	case DLT_RAW:
		l2_len = 0;
		break;

	case DLT_EN10MB:
		ethhdr = (struct eth_hdr *) frame;
		switch (ntohs(ethhdr->_type))
		{
		case 0x8100: /* IEEE 802.1Q VLAN tagging */
			l2_len = 18;
			break;
		default:
			l2_len = 14;
			break;
		}
		break;

	case DLT_LINUX_SLL:
		l2_len = 16;
		break;

	default:
		l2_len = -1;
		break;
	}

	if (-1 != l2_len)
	{
		// notice we assume there's no padding between level2 header and the IP header.
		// A bug would arise here if the above assumption was not met.
		return frame + l2_len;
	}
	else
	{
		return NULL;
	}
}

int detect_l2head_len(const char *frame)
{
	struct iphdr* iphdr;
	int8_t buff[60];
	int32_t len;
	uint16_t sum, checksum;
	int32_t offsets[] =
	{ 14, 0, 18 };

	for (int32_t i = 0; i < sizeof(offsets) / sizeof(int32_t); i++)
	{
		iphdr = (struct iphdr*) (frame + offsets[i]);
		if (iphdr->version != 4)
			continue;
		len = iphdr->ihl << 2;
		memcpy(buff, iphdr, len);
		sum = *(uint16_t*) (buff + 10);
		checksum = compute_ip_checksum((struct iphdr*) buff);
		if (checksum == sum)
			return offsets[i];
	}
	return -1;
}

unsigned short generate_the_port(unsigned short ori_src_port)
{
	static unsigned short rand = random();
	bool b = g_configuration.get_onoff_random_port();
	if (b)
	{
		return ((ori_src_port + rand) % 30000) + 4096;
	}
	else
	{
		return ori_src_port;
	}
}
