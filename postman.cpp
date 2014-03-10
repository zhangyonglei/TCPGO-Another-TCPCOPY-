/*********************************************
 * sniffer.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 10 Mar, 2014
 ********************************************/

#include "postman.h"
#include "postoffice.h"

postman::postman(postoffice* office)
{
	_office = office;
}

postman::~postman()
{
}

rawsock_postman::rawsock_postman(postoffice *office) : postman(office)
{
	int on, ret, flags;
	const char* err_hint;

	err_hint = "";

	// used to send IPs package on IP level.
	_send_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (_send_fd == -1)
	{
		err_hint = "root permission is required ";
		goto _err;
	}

	on = 1;
	if (setsockopt(_send_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		goto _err;
	}

	// on level2, sniff datalink package enclosing IP package as playload.
	// failed to capture outgoing packets elicited by incoming pakcets from other machines.
	// it actually works to capture both incoming and outgoing packets if the outgoing
	// pakcets are elicited by packets sent from the same machine.
	// _recv_fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));

	// code from tcpcopy, failed to capture outgoing packets.
	// _recv_fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);

	// code from http://www.binarytides.com/packet-sniffer-code-in-c-using-linux-sockets-bsd-part-2/.
	// works for both directions. But ethernet header is also received.
	_recv_fd = socket(AF_PACKET , SOCK_RAW , htons(ETH_P_ALL));

	// failed to capture outgoing packets. ethernet header is received.
	// _recv_fd = socket(AF_PACKET , SOCK_RAW , htons(ETH_P_IP));
	if (_recv_fd == -1)
	{
		err_hint = "root permission is required ";
		goto _err;
	}

	flags = fcntl(_recv_fd, F_GETFL, 0);
	if (flags < 0)
	{
		goto _err;
	}

	if (fcntl(_recv_fd, F_SETFL, flags|O_NONBLOCK) < 0)
	{
		goto _err;
	}

	return;

_err:
	// I'm supprised strerror_r doesn't work. buff is not filled with error infomation.
	//strerror_r(errno, _buff, sizeof(_buff));
	// The mentioned problem above is clear.
	// The GNU-specific strerror_r() returns a pointer to a string containing the error message.
	// This may be either a pointer to a string that the function stores in buf, or a pointer to
	// some (immutable) static string (in which case buf is unused).
	//g_logger.printf("%s\n", _buff);

	perror(err_hint);
	abort();
}

rawsock_postman::~rawsock_postman()
{
	close(_send_fd);
	close(_recv_fd);
}

int rawsock_postman::get_ready()
{
	g_poller.register_evt(_send_fd, mypoller::MYPOLLOUT, this);
	g_poller.register_evt(_recv_fd, mypoller::MYPOLLIN, this);
}

int rawsock_postman::recv(char buff[], int len)
{
	int ret;

	ret = ::recv(_recv_fd, buff, len, 0);

	return ret;
}

int rawsock_postman::sendto(const char* data, int len, const struct sockaddr *dest_addr, int addrlen)
{
	int ret;

	ret = ::sendto(_send_fd, data, len, MSG_DONTWAIT, (struct sockaddr *)dest_addr, addrlen);

	return ret;
}

void rawsock_postman::one_shot_timer_event_run()
{
	g_poller.register_evt(_send_fd, mypoller::MYPOLLOUT, this);
}

void rawsock_postman::punish_sender(int tickcount)
{
	g_poller.deregister_evt(_send_fd);
	g_timer.register_one_shot_timer_event(this, tickcount);
}

void rawsock_postman::pollin_handler(int fd)
{
	assert(fd == _recv_fd);

	get_postoffice()->recv_packets_from_wire();
}

void rawsock_postman::pollout_handler(int fd)
{
	assert(fd == _send_fd);

	get_postoffice()->send_packets_to_wire();
}

/*
pcap_postman::pcap_postman()
{
}

pcap_postman::~pcap_postman()
{
}
*/
