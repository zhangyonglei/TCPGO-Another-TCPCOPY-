/*********************************************
 * utils.cpp
 * Author: kamuszhou www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#include "misc.h"

struct eth_hdr
{
	uint8_t   _dest[6];
	uint8_t   _src[6];
	uint16_t  _type;
};

unsigned short calc_ip_checksum(const unsigned char* ip, int len)
{
	unsigned int sum = 0;

	while(len > 1){
		sum += *(unsigned short*)ip;
		ip += 2;
		if(sum & 0x80000000)   /* if high order bit set, fold */
			sum = (sum & 0xFFFF) + (sum >> 16);
		len -= 2;
	}

	if(len)       /* take care of left over byte */
		sum += (unsigned short) *(unsigned char *)ip;

	while(sum>>16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return ~sum;
}

/*
unsigned short calc_ip_checksum(const unsigned short buff[], int len_ip_header)
{
	int i;
	unsigned short word16;
	unsigned int sum;

	sum = 0;

	// make 16 bit words out of every two adjacent 8 bit words in the packet
	// and add them up
	for (i=0;i<len_ip_header;i=i+2){
		word16 =((buff[i]<<8)&0xFF00)+(buff[i+1]&0xFF);
		sum = sum + (unsigned int) word16;	
	}

	// take only 16 bits out of the 32 bit sum and add up the carries
	while (sum>>16)
		sum = (sum & 0xFFFF)+(sum >> 16);

	// one's complement the result
	sum = ~sum;

	return sum;
}*/

const unsigned char* strip_l2head(pcap_t *pcap, const unsigned char *frame)
{
	int l2_len;
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

int detect_l2head_len(const unsigned char *frame)
{
	struct iphdr* iphdr;
	char buff[60];
	int len;
	unsigned short sum, checksum;
	int offsets[4] = {14, 0, 18};

	for (int i = 0; i < sizeof(offsets)/sizeof(int); i++)
	{
		iphdr = (struct iphdr*)(frame + offsets[i]);
		if (iphdr->version != 4)
			continue;
		len = iphdr->ihl << 2;
		memcpy(buff, iphdr, len);
		sum = *(unsigned short*)(buff+10);
		buff[10] = 0;
		buff[11] = 0;
		checksum = calc_ip_checksum((unsigned char*)buff, len);
		if (checksum == sum)
			return offsets[i];
	}
	return -1;
}
