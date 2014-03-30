/*********************************************
 * postoffice.h
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: 11 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _POSTOFFICE_H_
#define _POSTOFFICE_H_

#include "utils.h"
#include "ip_pkt.h"
#include "thetimer.h"
#include "listmap.h"
#include "postman.h"

class postoffice_callback_interface
{
public:
	/**
	 * @param pkts on return, vector pkts are supposed to hold the packets which are expected to be sent
	 * and any previous data in the vector before calling is expelled.
	 * @return if return value is greater than or equal to zero, it indicates how many packets should be sent.
	 * or else, it will return IGNORE to indicate to ignore this postoffice_callback_interface
	 * return REMOVE to indicate to remove this postoffice_callback_interface.
	 * other return values are invalidate.
	 */
	enum {/*IGNORE = -100* obsoleted.*/ REMOVE = -101};
	virtual int pls_send_these_packets(std::vector<boost::shared_ptr<ip_pkt> >& pkts) = 0;

	/**
	 * @param ip_pkt points to the coming packet.
	 */
	virtual void got_a_packet(boost::shared_ptr<ip_pkt> pkt) = 0;

protected:
	virtual ~postoffice_callback_interface(){}
};

/**
 * This class is in charge of sending IP packets and receiving
 * datalink level packets.
 */
class postoffice
{
private:
	postoffice(int asio_idx);

public:
	virtual ~postoffice();

public:
	static postoffice& instance(int idx);

	void register_callback(uint64_t key, postoffice_callback_interface* callback);
	void deregister_callback(uint64_t key);

	void recv_packets_from_wire();
	void send_packets_to_wire();

	void get_ready(boost::shared_ptr<postman> pm);

private:
	void employ_a_postman(boost::shared_ptr<postman> pm);

private:
	struct in_addr _svr_addr;  ///< server's IP address. AKA: the dest IP address where the packets will be sent.
	int  _svr_port; ///< captured outbound IP packets don't matching the this port will be ignored. in network byte order.
	int _l2hdr_len;   ///< it's usually 14.

	typedef listmap<uint64_t, postoffice_callback_interface*> mylistmap;
	mylistmap _callbacks;

	boost::shared_ptr<postman> _postman;  ///< only one post man in this post office.

	int _asio_idx;
	int _asio_thrd_num;

	static boost::mutex _mutex;
	static std::vector<boost::shared_ptr<postoffice> > _postoffices;
};

#endif /* _POSTOFFICE_H_ */
