/*********************************************
 * sniffer.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 10 Mar, 2014
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

	flags = fcntl(_send_fd, F_GETFL, 0);
	if (flags < 0)
	{
		goto _err;
	}

	if (fcntl(_send_fd, F_SETFL, flags|O_NONBLOCK) < 0)
	{
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

pcap_postman::pcap_postman(postoffice* office) : postman(office)
{
	_pcap_handle = NULL;
}

pcap_postman::~pcap_postman()
{
	if (NULL != _pcap_handle)
		pcap_close(_pcap_handle);
}

int pcap_postman::get_ready()
{
	std::ostringstream ss;

	if (NULL != _pcap_handle)
		pcap_close(_pcap_handle);

	_pcap_handle = pcap_open_live(NULL, 65535, 0, 0, _errbuf);
	if (NULL == _pcap_handle)
	{
		g_logger.printf(_errbuf);
		abort();
	}

	ss << "tcp and src port " << g_configuration.get_dst_port();
    if (-1 == pcap_compile(_pcap_handle, &_filter, (char*)ss.str().c_str(), 0, 0))
    {
    	g_logger.printf("%s\n", pcap_geterr(_pcap_handle));
        abort();
    }

    if (-1 == pcap_setfilter(_pcap_handle, &_filter))
    {
    	g_logger.printf("Failed to set pcap filter: %s\n", ss.str().c_str());
    	pcap_freecode(&_filter);
    	abort();
    }

    pcap_freecode(&_filter);

    _recv_fd = pcap_get_selectable_fd(_pcap_handle);

    if (_recv_fd == -1)
    {
        g_logger.printf("pcap_get_selectable_fd failed.\n");
        abort();
    }

    if (-1 == pcap_setnonblock(_pcap_handle, 1/*nonblock is on*/, _errbuf))
    {
        g_logger.printf("%s\n", _errbuf);
        abort();
    }

	_send_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (_send_fd == -1)
	{
		perror("socket");
		abort();
	}

	int on = 1;
	if (setsockopt(_send_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	{
		perror("setsockopt");
		abort();
	}

	int flags;
	flags = fcntl(_send_fd, F_GETFL, 0);
	if (flags < 0)
	{
		perror("fcntl");
		abort();
	}

	if (fcntl(_send_fd, F_SETFL, flags|O_NONBLOCK) < 0)
	{
		perror("fcntl");
		abort();
	}

	_truncated_pkt_count = 0;

	g_poller.register_evt(_send_fd, mypoller::MYPOLLOUT, this);
	g_poller.register_evt(_recv_fd, mypoller::MYPOLLIN, this);

	return 0;
}

int pcap_postman::recv(char buff[], int len)
{
	struct pcap_pkthdr pkt_hdr;
	const char* pkt_data;
	const char* ip_pkt;
	int caplen;

	pkt_data = (const char*)pcap_next(_pcap_handle, &pkt_hdr);
	if (pkt_data != NULL)
	{
		caplen = pkt_hdr.caplen;
		if (caplen < pkt_hdr.len)
		{
			g_logger.printf("%d truncated packets are detected.\n",
					++_truncated_pkt_count);
			return -1;
		}
		else
		{
			ip_pkt = strip_l2head(_pcap_handle, pkt_data);
			if (ip_pkt != NULL && len >= caplen)
			{
				memcpy(buff, ip_pkt, caplen);
			}
		}

		return caplen;
	}
	else
	{
		return -1;
	}
}

int pcap_postman::sendto(const char* data, int len, const struct sockaddr *dest_addr, int addrlen)
{
	int ret;

	ret = ::sendto(_send_fd, data, len, MSG_DONTWAIT, (struct sockaddr *)dest_addr, addrlen);

	return ret;
}

void pcap_postman::one_shot_timer_event_run()
{
	g_poller.register_evt(_send_fd, mypoller::MYPOLLOUT, this);
}

void pcap_postman::punish_sender(int tickcount)
{
	g_poller.deregister_evt(_send_fd);
	g_timer.register_one_shot_timer_event(this, tickcount);
}

void pcap_postman::pollin_handler(int fd)
{
	assert(fd == _recv_fd);

	get_postoffice()->recv_packets_from_wire();
}

void pcap_postman::pollout_handler(int fd)
{
	assert(fd == _send_fd);

	get_postoffice()->send_packets_to_wire();
}

tcp_postman::tcp_postman(postoffice* office) : postman(office)
{
	_send_fd = -1;
	_listening_fd = -1;
	_conn_fd = -1;
	_buffer_used_len = 0;
}

tcp_postman::~tcp_postman()
{
	if (-1 != _send_fd)
		close(_send_fd);

	if (-1 != _listening_fd)
		close(_listening_fd);

	if (-1 != _conn_fd)
		close(_conn_fd);
}

int tcp_postman::get_ready()
{
	int opt, ret;
	socklen_t len;
	struct sockaddr_in addr;

	assert(-1 == _send_fd);
	assert(-1 == _listening_fd);
	assert(-1 == _conn_fd);
	assert( 0 == _buffer_used_len);

	_listening_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listening_fd == -1)
	{
		perror("socket");
		abort();
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_port = ntohs(_listening_port);
	addr.sin_family = AF_INET;

	opt = 1;
	ret = setsockopt(_listening_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt");
		abort();
	}

	if (bind(_listening_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		abort();
	}

	if (listen(_listening_fd, 3) == -1)
	{
		perror("listen");
		abort();
	}

	_send_fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (_send_fd == -1)
	{
		perror("socket");
		abort();
	}

	int on = 1;
	if (setsockopt(_send_fd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0)
	{
		perror("setsockopt");
		abort();
	}

	int flags;
	flags = fcntl(_send_fd, F_GETFL, 0);
	if (flags < 0)
	{
		perror("fcntl");
		abort();
	}

	if (fcntl(_send_fd, F_SETFL, flags|O_NONBLOCK) < 0)
	{
		perror("fcntl");
		abort();
	}

	g_poller.register_evt(_listening_fd, mypoller::MYPOLLIN, this);
	g_poller.register_evt(_send_fd, mypoller::MYPOLLOUT, this);

	return 0;
}

int tcp_postman::recv(char buff[], int len)
{
	bool exist;
	int buff_len;

	buff_len = len;
	exist = parse_buffer_and_get_ip_pkt(buff, buff_len);

	if (exist)
		return buff_len;
	else
		return -1;
}

int tcp_postman::sendto(const char* data, int len, const struct sockaddr *dest_addr, int addrlen)
{
	int ret;

	ret = ::sendto(_send_fd, data, len, MSG_DONTWAIT, (struct sockaddr *)dest_addr, addrlen);

	return ret;
}

void tcp_postman::one_shot_timer_event_run()
{
	g_poller.register_evt(_send_fd, mypoller::MYPOLLOUT, this);
}

void tcp_postman::punish_sender(int tickcount)
{
	g_poller.deregister_evt(_send_fd);
	g_timer.register_one_shot_timer_event(this, tickcount);
}

void tcp_postman::pollin_handler(int fd)
{
	if (fd == _conn_fd)
	{
		save_peer_response_to_buffer();
		get_postoffice()->recv_packets_from_wire();
	}
	else if (fd == _listening_fd)
	{
		if (0 < _conn_fd)
		{
			g_poller.deregister_evt(_conn_fd);
			close(_conn_fd);
		}

		_conn_fd = accept(_listening_fd, NULL, NULL);
		if (_conn_fd < 0)
		{
			perror("accept");
		}

		int flags;
		flags = fcntl(_conn_fd, F_GETFL, 0);
		if (flags < 0)
		{
			perror("fcntl");
			abort();
		}

		if (fcntl(_conn_fd, F_SETFL, flags|O_NONBLOCK) < 0)
		{
			perror("fcntl");
			abort();
		}

		g_poller.register_evt(_conn_fd, mypoller::MYPOLLIN, this);
	}
}

void tcp_postman::pollout_handler(int fd)
{
	assert(fd == _send_fd);

	get_postoffice()->send_packets_to_wire();
}

void tcp_postman::save_peer_response_to_buffer()
{
	int i, ret;
	char* buff_ptr;
	int buff_available_len;
	char buff[1024];

	buff_ptr = _buffer_block + _buffer_used_len;
	buff_available_len = _buffer_block_len - _buffer_used_len;
	if (0 == buff_available_len)
	{
		int tmp;
		bool exist = parse_buffer_and_get_ip_pkt(NULL, tmp);
		if (exist)
		{
			g_logger.printf("tcp_postman\'s buffer for peer\'s response is full.\n");
			return;
		}

		// when code reaches here, the buffer is full and no IP packets are detected.
		// Then, let the code continue. The following read system call will return 0
		// and this kind of exception will be handled well.
	}
	ret = read(_conn_fd, buff_ptr, buff_available_len);
	if (ret > 0)
	{
		boost::shared_ptr<cascade::MemBlock> mem_block = boost::make_shared<cascade::MemBlock>(ret);
		memcpy(mem_block->data(), buff_ptr, ret);
		g_cascade.push_back(mem_block);

		_buffer_used_len += ret;
	}
	else if (ret <= 0)
	{
		if (ret == 0 && buff_available_len == 0)
		{
			const char* hint = "Make sure the two options have been specified:\n"
				"1. -s 0 was specified to avoid truncated IP packages.\n"
				"2. -w - was specified to output captured IP packages in binary format.\n";
			write(_conn_fd, hint, strlen(hint));
		}
		else
		{
			char *ptr = strerror_r(errno, buff, sizeof(buff));
			write(_conn_fd, ptr, strlen(ptr));
		}
		close(_conn_fd);
		_conn_fd = -1;
	}
}

bool tcp_postman::parse_buffer_and_get_ip_pkt(char ip_pkt[], int& len)
{
	char* buff_ptr;
	struct iphdr* iphdr;
	struct tcphdr* tcphdr;
	int buff_len, i, iphdr_len, ip_tot_len;
	int sentinel;
	bool exist;
	uint16_t src_port;
	uint16_t sum, checksum;
	uint16_t dst_port_in_netbyte_order;

	buff_ptr = _buffer_block;
	buff_len = _buffer_used_len;
	sentinel = 0;
	exist = false;

	dst_port_in_netbyte_order = htons(g_configuration.get_dst_port());

	for (i = 0; i <= buff_len - 40;)
	{
		char ch;
		char* ptr;
		ptr = buff_ptr + i;
		ch = (*ptr & 0xf0);

		if (ch != 0x40)
		{
			i++;
			continue;
		}

		iphdr = (struct iphdr*)ptr;
		if(iphdr->version != 4)
		{
			i++;
			continue;
		}
		iphdr_len = iphdr->ihl << 2;
		sum = iphdr->check;
		checksum = compute_ip_checksum(iphdr);
		iphdr->check = sum;
		if (checksum != sum)
		{
			i++;
			continue;
		}
		ip_tot_len = ntohs(iphdr->tot_len);
		if (buff_len - i < ip_tot_len)
		{
			break;
		}
		tcphdr = (struct tcphdr*)(ptr + iphdr->ihl*4);
		sum = tcphdr->check;
		checksum = compute_tcp_checksum(iphdr, tcphdr);
		tcphdr->check = sum;
		if (checksum != sum)
		{
			// TODO. It's weird the checksum always failed here.
			// I just let it pass as an expedient at present.
//			i++;
//			continue;
		}
		if (tcphdr->source != dst_port_in_netbyte_order)
		{
			i++;
			continue;
		}
		exist = true;
		if (NULL != ip_pkt)
		{
			assert(ip_tot_len <= len);
			memcpy(ip_pkt, ptr, ip_tot_len);
			len = ip_tot_len;
		}
		else
		{
			goto _exit;
		}

		i += ip_tot_len;
		sentinel = i;

		break;
	}
	if (0 != sentinel)
	{
		int remaining_data_len;
		remaining_data_len = buff_len - sentinel;
		if (remaining_data_len > 0)
		{
			memmove(buff_ptr, buff_ptr + sentinel, remaining_data_len);
		}
		_buffer_used_len = remaining_data_len;
	}

_exit:
	return exist;
}
