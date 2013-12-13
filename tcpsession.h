/*********************************************
 * tcpsession.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#ifndef _TCPSESSION_H_
#define _TCPSESSION_H_

#include <list>
#include "ip_pkt.h"

class tcpsession {
public:
	tcpsession();
	virtual ~tcpsession();

	/**
	 * add a ip packet to this session as a sample.
	 */
	void append_ip_sample(const unsigned char* ippkt);


	/**
	 * @return return 0 if this session is healthy, return error code otherwise.
	 */
	int verify();

private:
	std::list<ip_pkt>  _ippkts_samples;    ///< The ip packages which will be used to emulate the pseudo-client.
	std::list<ip_pkt>  _ippkts_received;   ///< The ip packages received from the server will be saved here.
};

#endif /* _TCPSESSION_H_ */
