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

