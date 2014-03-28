/*********************************************
 * postman.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 10 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include <boost/make_shared.hpp>
#include "cute_logger.h"
#include "postman.h"
#include "postoffice.h"
#include "cascade.h"

postman::postman(postoffice* office)
{
	_office = office;
}

postman::~postman()
{
	_done_recv_thrd = true;
	_done_snd_thrd = true;

	close(_send_fd);

	_recv_thrd.join();
	_send_thrd.join();
}

void postman::get_ready()
{
	const char* err_hint;
	int on;

	_count_recv_queue = 0;
	_count_snd_queue = 0;

	_done_recv_thrd = false;
	_done_snd_thrd = false;

	_send_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (_send_fd == -1)
	{
		err_hint = "socket";
		goto _err;
	}

	on = 1;
	if (setsockopt(_send_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	{
		err_hint = "setsockopt";
		goto _err;
	}

	// use blocking file descriptor.
//	flags = fcntl(_send_fd, F_GETFL, 0);
//	if (flags < 0)
//	{
//		err_hint = "fcntl";
//		goto _err;
//	}
//
//	if (fcntl(_send_fd, F_SETFL, flags|O_NONBLOCK) < 0)
//	{
//		err_hint = "fcntl";
//		goto _err;
//	}

	get_ready4subclass();

	_recv_thrd = boost::thread(boost::bind(&postman::recv_thrd_entry, this));
	_send_thrd = boost::thread(boost::bind(&postman::send_thrd_entry, this));

	return;

_err:
	perror(err_hint);
	abort();
}

bool postman::recv(boost::shared_ptr<ip_pkt>& pkt)
{
	bool success;

	success = _recv_queue.pop(pkt);

	if (success)
	{
		_count_recv_queue--;
	}

	return success;
}

bool postman::send(boost::shared_ptr<ip_pkt> pkt)
{
	bool success;

	success = _snd_queue.push(pkt);

	if (success)
	{
		_count_snd_queue++;
	}

	return success;
}

void postman::recv_thrd_entry()
{
	while (!_done_recv_thrd)
	{
		recv_impl();
	}
}

void postman::send_thrd_entry()
{
	while (!_done_snd_thrd)
	{
		send_impl();
	}
}

void postman::send_impl()
{
	bool success;
	boost::shared_ptr<ip_pkt> pkt;
	struct sockaddr_in dst_addr;
	const char* starting_addr;
	int tot_len;

	do
	{
		success = _snd_queue.pop(pkt);
		if (success)
		{
			dst_addr.sin_family = AF_INET;
			dst_addr.sin_addr.s_addr = pkt->get_iphdr()->daddr;

			starting_addr = pkt->get_starting_addr();
			tot_len = pkt->get_tot_len();

			sendto(_send_fd, starting_addr, tot_len, 0,
					reinterpret_cast<struct sockaddr*>(&dst_addr), sizeof(dst_addr));

			_count_snd_queue--;
		}
	}while(success);
}

void postman::push_recved_ippkt(boost::shared_ptr<ip_pkt> pkt)
{
	bool success;
	while (true && !_done_recv_thrd)
	{
		int pkt_tot_len = pkt->get_tot_len();
		const char* pkt_addr = pkt->get_starting_addr();
		boost::shared_ptr<MemBlock> mem_block = boost::make_shared<MemBlock>(pkt_tot_len);
		memcpy(mem_block->data(), pkt_addr, pkt_tot_len);
		g_cascade.push_back(mem_block);

		success = _recv_queue.push(pkt);
		if (success)
		{
			_count_recv_queue++;
			break;
		}
		else
		{
			boost::this_thread::sleep(boost::posix_time::milliseconds(1));
		}
	}
}
