/*********************************************
 * sender.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: 11 Dec, 2013
 ********************************************/

#ifndef _POSTMAN_H_
#define _POSTMAN_H_

#include <list>

class postoffice;
extern postoffice g_postman;

/**
 * This interface is employed by class postoffice to
 * inform the coming packet received from datalink.
 */
class recv_evt_listener
{
public:
	virtual int check(const char *ip_pkt) = 0;
	virtual ~recv_evt_listener(){};
};

/**
 * This class is in charge of sending IP packets and receiving
 * datalink level packets.
 */
class postoffice {
public:
	postoffice();
	virtual ~postoffice();

	/**
	 * @return return 0 on success, otherwise return -1 on error.
	 */
	int send(const unsigned char* data, int len);

	/**
	 *@param port in host byte order.
	 */
	void set_svr_port(unsigned short port);

	void recv();

	/**
	 * @param addr The dest IP address in c-string.
	 */
	int set_svr_addr(const char* addr);

	void append_recv_evt_listener(recv_evt_listener* listener);

private:
	struct sockaddr_in  _svr_addr;  ///< server's IP address. AKA: the dest IP address where the packets will be sent.
	int  _svr_port; ///< captured outbound IP packets don't matching the this port will be ignored. in network byte order.
	int  _send_fd;   ///< The raw socket file descriptor used to send IP packets. on level 3.
	int  _recv_fd;   ///< The raw socket file descriptor created on datalink level to sniff traffic. on level 2.
	char  _buff[8192];   ///< self-explanatory.

	std::list<recv_evt_listener*> _listeners;  ///< listeners will be informed the outbound traffic.
};

#endif /* _POSTMAN_H_ */
