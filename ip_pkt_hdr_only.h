/*********************************************
 * ip_pkt_hdr_only.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 28 Mar, 2014
 ********************************************/

#ifndef _IP_PKT_HDR_ONLY_H_
#define _IP_PKT_HDR_ONLY_H_

#include "ip_pkt.h"

class ip_pkt_hdr_only : public ip_pkt
{
public:
	ip_pkt_hdr_only();
	ip_pkt_hdr_only(const char* pkt);

	virtual ~ip_pkt_hdr_only();

public:
	virtual void cp(const char* pkt);

private:
	// not allowed to invoke this method 'cos this is a IP header & TCP header only packet.
	virtual const char* get_tcp_payload()const
	{
		// not supposed to reach here
		abort();
		return NULL;
	}
};

#endif /* _IP_PKT_HDR_ONLY_H_ */
