/*********************************************
 * sender.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: 11 Dec, 2013
 ********************************************/

#include "misc.h"
#include "utils.h"
#include "cute_logger.h"
#include "postoffice.h"

#define SNAP_LEN 8192

using namespace std;

postoffice g_postman;

postoffice::postoffice()
{
	int32_t on;
	int32_t ret;

	// used to send IPs package on IP level.
	_send_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (_send_fd == -1)
	{
		goto _err;
	}

	on = 1;
	if (setsockopt(_send_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		goto _err;
	}

	// on level2, sniff datalink package enclosing IP package as playload.
	_recv_fd = socket(PF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));
	if (_recv_fd == -1)
	{
		goto _err;
	}

	set_svr_port(80);
	ret = set_svr_addr("127.0.0.1");
	assert(ret != -1);

	return;

_err:
    // I'm supprised strerror_r doesn't work. buff is not filled with error infomation.
	//strerror_r(errno, _buff, sizeof(_buff));
	//g_logger.printf("%s\n", _buff);

	perror("root permission is required ");
	abort();
}

postoffice::~postoffice()
{
	close(_send_fd);
	close(_recv_fd);
}

int32_t postoffice::send(const uint8_t* data, int32_t len)
{
	int32_t ret;
	ret = sendto(_send_fd, data, len, MSG_DONTWAIT,
			(const struct sockaddr *)&_svr_addr, sizeof(_svr_addr));
	if (ret == len)
		return 0;
	else
		return -1;
}

void postoffice::recv()
{
	int32_t ret;

	ret = ::recv(_recv_fd, _buff, sizeof(_buff), 0);

	ip_packet_parser(_buff);

	// ignore the un-expected packages.
	if (ip_tot_len != ret)
	{
		g_logger.printf("Ignore a truncated packet.");
		return;
	}

	if (iphdr->protocol != IPPROTO_TCP)
		return;

	if (_svr_port != tcphdr->source)
		return;

	// now inform the listeners the coming ip package.
	for (list<recv_evt_listener*>::const_iterator ite = _listeners.begin();
		ite != _listeners.end();
		++ite)
	{
		(*ite)->check(_buff);
	}
}

void postoffice::set_svr_port(uint16_t port)
{
	_svr_port = htons(port);
}

int32_t postoffice::set_svr_addr(const char* addr)
{
	int32_t ret;
	assert(addr != NULL);
    memset(&_svr_addr, 0, sizeof(_svr_addr));
    _svr_addr.sin_family = AF_INET;
    ret = inet_aton(addr, &_svr_addr.sin_addr);
    if (INADDR_NONE == ret)
    	return -1;
    else
    	return 0;
}

void postoffice::append_recv_evt_listener(recv_evt_listener* listener)
{
	_listeners.push_back(listener);
}
