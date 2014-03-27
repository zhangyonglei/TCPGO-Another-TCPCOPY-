/*********************************************
 * realtime_captureer.cpp
 * Author: kamuszhou@tencent.com 16236914@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 27 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "misc.h"
#include "realtime_captureer.h"
#include "cute_logger.h"
#include "session_manager.h"

realtime_captureer g_realtime_captureer;

realtime_captureer::realtime_captureer()
{
	_traffic_listening_port = 1993;
}

realtime_captureer::~realtime_captureer()
{
}

int realtime_captureer::get_ready()
{
	int ret;
	int i, opt;
	socklen_t len;
	struct sockaddr_in addr;

	_traffic_listening_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_traffic_listening_fd == -1)
	{
		perror("socket");
		abort();
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_port = ntohs(_traffic_listening_port);
	addr.sin_family = AF_INET;

	opt = 1;
	ret = setsockopt(_traffic_listening_fd, SOL_SOCKET, SO_REUSEADDR, &opt,
			sizeof(opt));
	if (ret == -1)
	{
		perror("setsockopt");
		abort();
	}

	if (bind(_traffic_listening_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		abort();
	}

	if (listen(_traffic_listening_fd, 10) == -1)
	{
		perror("listen");
		abort();
	}

	for (i = 0; i < CONNECTED_FD_SIZE; i++)
	{
		_conns[i]._fd = -1;
		_conns[i]._buffer_block = NULL;
		_conns[i]._buffer_used_len = 0;
	}

	g_reactor.register_evt(_traffic_listening_fd, reactor::MYPOLLIN, this);

	return 0;
}

void realtime_captureer::pollin_handler(int fd)
{
	int connected_fd;

	if (_traffic_listening_fd == fd)
	{
		accept_conn();
	}
	else
	{
		readin_traffic(fd);
	}
}

void realtime_captureer::pollout_handler(int fd)
{
}

void realtime_captureer::accept_conn()
{
	int i;
	int fd;
	conn_info* conn;

	fd = ::accept(_traffic_listening_fd, NULL, NULL);
	if (fd < 0)
	{
		perror("accept");
		return;
	}

	for (i = 0; i < CONNECTED_FD_SIZE; i++)
	{
		if (_conns[i]._fd == -1)
			break;
	}

	if (i == CONNECTED_FD_SIZE)
	{
		// no rooms to store a new connection.
		close(fd);
		return;
	}

	conn = &_conns[i];
	assert(conn->_buffer_block == NULL);
	conn->_fd = fd;
	conn->_buffer_block = new char[BUFFER_LEN_FOR_TRAFFIC];
	assert(conn->_buffer_block != NULL);
	memset(conn->_buffer_block, 0, BUFFER_LEN_FOR_TRAFFIC);
	conn->_buffer_used_len = 0;
	g_reactor.register_evt(fd, reactor::MYPOLLIN, this);
}

void realtime_captureer::readin_traffic(int fd)
{
	int i, ret;
	char* buff_ptr;
	int buff_available_len;
	conn_info* _conn;

	i = map_fd_to_index(fd);
	_conn = &_conns[i];
	buff_ptr = _conn->_buffer_block + _conn->_buffer_used_len;
	buff_available_len = BUFFER_LEN_FOR_TRAFFIC - _conn->_buffer_used_len;
	ret = read(fd, buff_ptr, buff_available_len);
	if (ret > 0)
	{
		_conn->_buffer_used_len += ret;
		parse_buff_and_get_ip_pkts(i);
	}
	else if (ret <= 0)
	{
		if (ret == 0 && buff_available_len == 0)
		{
			const char* hint = "Make sure the two options have been specified:\n"
				"1. -s 0 was specified to avoid truncated IP packages.\n"
				"2. -w - was specified to output captured IP packages in binary format.\n";
			write(fd, hint, strlen(hint));
		}
		else
		{
			char buff[1024];
			char *ptr = strerror_r(errno, buff, sizeof(buff));
			write(fd, ptr, strlen(ptr));
		}
		close_fd(fd);
	}
}

void realtime_captureer::close_fd(int fd)
{
	int i;
	conn_info* conn;

	i = map_fd_to_index(fd);
	conn = &_conns[i];
	close(conn->_fd);
	delete conn->_buffer_block;
	conn->_fd = -1;
	conn->_buffer_block = NULL;
	conn->_buffer_used_len = 0;
	g_logger.printf("fd %d closed.\n", fd);
}

int realtime_captureer::map_fd_to_index(int fd)
{
	int i;

	for (i = 0; i < CONNECTED_FD_SIZE; i++)
	{
		if (_conns[i]._fd == fd)
			break;
	}
	assert(i != CONNECTED_FD_SIZE);

	return i;
}

void realtime_captureer::parse_buff_and_get_ip_pkts(int index)
{
	char* buff_ptr;
	conn_info* conn;
	struct iphdr* iphdr;
	struct tcphdr* tcphdr;
	int buff_len, i, iphdr_len, ip_tot_len;
	int sentinel;
	uint16_t src_port;
	uint16_t sum, checksum;

	assert(index >=0 && index <= CONNECTED_FD_SIZE);
	conn = &_conns[index];
	buff_ptr = conn->_buffer_block;
	buff_len = conn->_buffer_used_len;
	sentinel = 0;

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
			i++;
			continue;
		}
		src_port = ntohs(tcphdr->source);
		tcphdr->source = htons(generate_the_port(src_port));

		// pluck out the incoming ip packet.
		boost::shared_ptr<ip_pkt> pkt = boost::make_shared<ip_pkt>(ptr);
		g_session_manager.inject_a_realtime_ippkt(pkt);
		i += ip_tot_len;
		sentinel = i;
	} // end of for loop.

	if (0 != sentinel)
	{
		// discards the outdated traffic.
		int remaining_data_len;
		remaining_data_len = buff_len - sentinel;
		assert(remaining_data_len >= 0);
		if (remaining_data_len > 0)
		{
			memmove(buff_ptr, buff_ptr + sentinel, remaining_data_len);
		}
		conn->_buffer_used_len = remaining_data_len;
	}
}
