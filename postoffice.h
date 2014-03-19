/*********************************************
 * sender.h
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: 11 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _POSTMAN_H_
#define _POSTMAN_H_

#include "utils.h"
#include "ip_pkt.h"
#include "thetimer.h"
#include "listmap.h"
#include "postman.h"
#include "mypoller.h"

class postoffice;
extern postoffice g_postoffice;

class postoffice_callback_interface
{
public:
	/**
	 * @param pkts on return, vector pkts are supposed to hold the packets which are expected to be sent
	 * and any previous data in the vector before calling is expelled.
	 * @return how many packets are expected to be sent.
	 */
	virtual int pls_send_these_packets(std::vector<const ip_pkt*>& pkts) = 0;

	/**
	 * @param ip_pkt points to the coming packet.
	 */
	virtual void got_a_packet(const ip_pkt *pkt) = 0;

protected:
	virtual ~postoffice_callback_interface(){}
};

/**
 * This class is in charge of sending IP packets and receiving
 * datalink level packets.
 */
class postoffice
{
public:
	postoffice();
	virtual ~postoffice();

	void get_ready();

	void register_callback(uint64_t key, postoffice_callback_interface* callback);
	void deregister_callback(uint64_t key);

	void recv_packets_from_wire();
	void send_packets_to_wire();

private:
	struct in_addr _svr_addr;  ///< server's IP address. AKA: the dest IP address where the packets will be sent.
	int  _svr_port; ///< captured outbound IP packets don't matching the this port will be ignored. in network byte order.
	char  _buff[4096*10];   ///< self-explanatory.
	int _l2hdr_len;   ///< it's usually 14.

	typedef listmap<uint64_t, postoffice_callback_interface*> mylistmap;
	mylistmap _callbacks;

	boost::shared_ptr<postman> _postman;  ///< only one post man in this post office.
};

#endif /* _POSTMAN_H_ */
