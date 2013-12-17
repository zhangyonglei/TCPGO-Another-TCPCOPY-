/*********************************************
 * tcpsession.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#ifndef _TCPSESSION_H_
#define _TCPSESSION_H_

#include <list>
#include "misc.h"
#include "postoffice.h"
#include "ip_pkt.h"

class tcpsession : public postoffice_callback_interface
{
public:
	/**
	 *@param port in network byte order.
	 */
	tcpsession(uint32_t ip, uint16_t port);
	virtual ~tcpsession();

	/**
	 * add a ip packet to this session as a sample.
	 */
	void append_ip_sample(const char* ippkt);

	/**
	 * This function will also remove ack packets without playload.
	 * @return return 0 if the integrity check is passed, otherwise return non-zero error code.
	 */
	int check_samples_integrity();

public:
	virtual const ip_pkt* pls_send_this_packet();

	virtual void got_a_packet(const ip_pkt *pkt);


private:
	std::list<ip_pkt>  _ippkts_samples;    ///< The ip packages which will be used to emulate the pseudo-client.
	std::list<ip_pkt>::iterator  _ite_next_avaliable;

	// reserved for the possible future version if i can still play with this stuff.
//	std::list<ip_pkt>  _ippkts_received;   // The ip packages received from the server will be saved here.

	uint32_t _client_src_ip_num;
	std::string _client_src_ip_str;
	uint16_t _client_src_port;      ///< in host byte order
};

#endif /* _TCPSESSION_H_ */
