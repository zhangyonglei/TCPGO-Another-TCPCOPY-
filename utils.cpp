/*********************************************
 * utils.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#include "misc.h"

struct eth_hdr
{
	uint8_t   _dest[6];
	uint8_t   _src[6];
	uint16_t  _type;
};

uint16_t calc_ip_checksum(const uint8_t* iphdr, int32_t hdrlen)
{
	uint32_t sum = 0;

	while(hdrlen > 1){
		sum += *(uint16_t*)iphdr;
		iphdr += 2;
		if(sum & 0x80000000)   /* if high order bit set, fold */
			sum = (sum & 0xFFFF) + (sum >> 16);
		hdrlen -= 2;
	}

	if(hdrlen)       /* take care of left over byte */
		sum += (uint16_t) *(uint8_t *)iphdr;

	while(sum>>16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return ~sum;
}

const uint8_t* strip_l2head(pcap_t *pcap, const uint8_t *frame)
{
	int32_t l2_len;
	struct eth_hdr *ethhdr;

	l2_len = -1;
	switch (pcap_datalink(pcap)) {
	case DLT_RAW:
		l2_len = 0;
		break;
	case DLT_EN10MB:
		ethhdr = (struct eth_hdr *)frame;
		switch (ntohs(ethhdr->_type)) {
		case  0x8100: /* IEEE 802.1Q VLAN tagging */
		l2_len = 18;
		break;
		default:
			l2_len = 14;
			break;
		}
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

int32_t detect_l2head_len(const uint8_t *frame)
{
	struct iphdr* iphdr;
	int8_t buff[60];
	int32_t len;
	uint16_t sum, checksum;
	int32_t offsets[4] = {14, 0, 18};

	for (int32_t i = 0; i < sizeof(offsets)/sizeof(int32_t); i++)
	{
		iphdr = (struct iphdr*)(frame + offsets[i]);
		if (iphdr->version != 4)
			continue;
		len = iphdr->ihl << 2;
		memcpy(buff, iphdr, len);
		sum = *(uint16_t*)(buff+10);
		buff[10] = 0;
		buff[11] = 0;
		checksum = calc_ip_checksum((uint8_t*)buff, len);
		if (checksum == sum)
			return offsets[i];
	}
	return -1;
}
