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

postoffice g_postoffice;

postoffice::postoffice()
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
	// pakcets are eclited by packets sent from the same machine.
	_recv_fd = socket(AF_PACKET, SOCK_DGRAM, htons(ETH_P_IP));

	// code from tcpcopy, failed to capture outgoing packets.
	// _recv_fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);

	// code from http://www.binarytides.com/packet-sniffer-code-in-c-using-linux-sockets-bsd-part-2/.
	// works for both directions. But ethernet header is also received.
	//_recv_fd = socket( AF_PACKET , SOCK_RAW , htons(ETH_P_ALL));

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

	set_svr_port(80);
	ret = set_svr_addr("127.0.0.1");
	assert(ret != -1);

	_l2hdr_len = -1;
	
	return;

_err:
	// I'm supprised strerror_r doesn't work. buff is not filled with error infomation.
	//strerror_r(errno, _buff, sizeof(_buff));
	//g_logger.printf("%s\n", _buff);

	perror(err_hint);
	abort();
}

postoffice::~postoffice()
{
	close(_send_fd);
	close(_recv_fd);
}

void postoffice::get_ready()
{
	g_poller.register_evt(_send_fd, poller::POLLOUT, this);
	g_poller.register_evt(_recv_fd, poller::POLLIN, this);
}

void postoffice::register_callback(uint64_t key, postoffice_callback_interface* callback)
{
	std::map<uint64_t, postoffice_callback_interface*>::iterator ite;
	ite = _callbacks.find(key);
	assert(ite == _callbacks.end());
	_callbacks[key] = callback;
}

void postoffice::deregister_callback(uint64_t key)
{
	std::map<uint64_t, postoffice_callback_interface*>::iterator ite;
	ite = _callbacks.find(key);
	assert(ite != _callbacks.end());
	_callbacks.erase(ite);
}

void postoffice::pollin_handler(int fd)
{
	int ret;
	uint64_t key;
	uint16_t src_port;
	char *ptr_ippkt;
	postoffice_callback_interface* callback;
	std::map<uint64_t, postoffice_callback_interface*>::iterator ite;

	if (fd != _recv_fd)
		return;

	while(true)
	{
		ret = ::recv(fd, _buff, sizeof(_buff), 0);
		if (ret < 0 && errno != EINTR)
			return;

		if (-1 == _l2hdr_len)
		{
			_l2hdr_len = detect_l2head_len(_buff);
			if (-1 == _l2hdr_len)
			{
				cerr << "Failed to detect data link level header's length.\n";
			}
		}

		ptr_ippkt = _buff + _l2hdr_len;
		ip_packet_parser(ptr_ippkt);
		// ip_packet_parser(_buff);

		// ignore the un-expected packages.
		if (ip_tot_len > ret)
		{
			g_logger.printf("Ignore a truncated packet.\n");
			cerr  << "Ignore a truncated packet.\n";
			continue;
		}

		if (iphdr->protocol != IPPROTO_TCP)
		{
			continue;
		}

		src_port = ntohs(tcphdr->source);
		g_logger.printf("Got a tcp packet with source port %hu.\n", src_port);
		if (_svr_port != tcphdr->source)
		{
			continue;
		}

		// now inform the corresponding receiver the coming ip package.
		key = mk_sess_key(iphdr->saddr, tcphdr->dest);
		ite = _callbacks.find(key);
		if (ite != _callbacks.end())
		{
			ip_pkt pkt(ptr_ippkt);
			ite->second->got_a_packet(&pkt);
		}
	}
}

void postoffice::pollout_handler(int fd)
{
	int ret;
	const ip_pkt* pkt;
	const char* starting_addr;
	int tot_len;
	std::map<uint64_t, postoffice_callback_interface*>::iterator ite;
	postoffice_callback_interface* callback;

	if (fd != _send_fd)
		return;

	for(ite = _callbacks.begin(); ite != _callbacks.end(); ++ite)
	{
		callback = ite->second;
		do
		{
			pkt = callback->send_this_packet();
			if (NULL == pkt)
			{
				break;
			}

			starting_addr = pkt->get_starting_addr();
			tot_len = pkt->get_tot_len();
			ret = send(_send_fd, starting_addr, tot_len, MSG_DONTWAIT);
			if (ret < 0 && errno == EINTR)
				return;
		}while(true);
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
