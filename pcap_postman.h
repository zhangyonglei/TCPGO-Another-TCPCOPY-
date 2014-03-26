/*********************************************
 * pcap_postman.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Mar, 2014
 ********************************************/

#ifndef _PCAP_POSTMAN_H_
#define _PCAP_POSTMAN_H_

#include "postman.h"

class pcap_postman : public postman
{
public:
	pcap_postman(postoffice* po);
	virtual ~pcap_postman();

private:
	virtual void get_ready4subclass();
	virtual void recv_impl();
};

#endif /* _PCAP_POSTMAN_H_ */
