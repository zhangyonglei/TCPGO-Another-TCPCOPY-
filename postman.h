/*********************************************
 * sniffer.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 10 Mar, 2014
 ********************************************/

#ifndef _SNIFFER_H_
#define _SNIFFER_H_

#include "misc.h"
#include "mypoller.h"
#include "thetimer.h"

class postoffice;

class postman : public evt_workhorse
{
public:
	postman(postoffice* office);
	virtual ~postman();

	/**
	 * return the internal file descriptor.
	 */
	virtual int get_ready() = 0;

	/**
	 * sniff the traffic.
	 */
	virtual int recv(char buff[], int len) = 0;

	/**
	 * send the packet.
	 */
	virtual int sendto(const char* data, int len, const struct sockaddr *dest_addr, int addrlen) = 0;

	/**
	 * suspend the sender for a moment.
	 */
	virtual void punish_sender(int tickcount) = 0;

	postoffice* get_postoffice()
	{
		return _office;
	}

private:
	postoffice* _office;  ///< the post office that the postman works for.
};

class rawsock_postman : public postman, public timer_event
{
public:
	rawsock_postman(postoffice* office);
	~rawsock_postman();

	virtual int get_ready();

	virtual int recv(char buff[], int len);

	virtual int sendto(const char* data, int len, const struct sockaddr *dest_addr, int addrlen);

	virtual void punish_sender(int tickcount);

	virtual void pollin_handler(int fd);
	virtual void pollout_handler(int fd);

public:
	void one_shot_timer_event_run();

private:
	int  _send_fd;   ///< The raw socket file descriptor used to send IP packets. on level 3.
	int  _recv_fd;   ///< The raw socket file descriptor created on datalink level to sniff traffic. on level 2.
};

class pcap_postman : public postman, public timer_event
{
public:
	pcap_postman(postoffice* office);
	~pcap_postman();

	virtual int get_ready();

	virtual int recv(char buff[], int len);

	virtual int sendto(const char* data, int len, const struct sockaddr *dest_addr, int addrlen);

	virtual void punish_sender(int tickcount);

	virtual void pollin_handler(int fd);
	virtual void pollout_handler(int fd);

public:
	void one_shot_timer_event_run();

private:
	struct bpf_program _filter;
	pcap_t* _pcap_handle;
	char _errbuf[PCAP_ERRBUF_SIZE];
	int _truncated_pkt_count;
	int _recv_fd;
	int _send_fd;
};

#endif /* _SNIFFER_H_ */
