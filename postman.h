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

class tcp_postman : public postman, public timer_event
{
public:
	tcp_postman(postoffice* office);
	~tcp_postman();

	virtual int get_ready();

	virtual int recv(char buff[], int len);

	virtual int sendto(const char* data, int len, const struct sockaddr *dest_addr, int addrlen);

	virtual void punish_sender(int tickcount);

	virtual void pollin_handler(int fd);
	virtual void pollout_handler(int fd);

public:
	void one_shot_timer_event_run();

private:
	void save_peer_response_to_buffer();

	/**
	 * If there exist a integral IP packets. The IP packet will be saved at the address
	 * pointed by ip_pkt[]. Notice if ip_pkt is NULL, no copying will be performed.
	 * len should be long enough because of no buffer flow check at present.
	 * On return len will be changed to denote how many bytes have been copied.
	 * Return true if there exist a integral IP packet. Otherwise return false.
	 */
	bool parse_buffer_and_get_ip_pkt(char ip_pkt[], int& len);

private:
	int _send_fd;
	int _listening_fd;
	int _conn_fd;
	static const int _listening_port = 1992;
	static const int _buffer_block_len = 4096*50;
	char _buffer_block[_buffer_block_len];
	int _buffer_used_len;
};

#endif /* _SNIFFER_H_ */
