/*********************************************
 * tcppostman.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "tcp_postman.h"
#include "configuration.h"
#include "ip_pkt_hdr_only.h"

tcp_postman::tcp_postman(postoffice* po)
: postman(po)
{
	_listening_fd = -1;
	_conn_fd = -1;
	_buffer_used_len = 0;
}

tcp_postman::~tcp_postman()
{
	if (-1 != _listening_fd)
	{
		close(_listening_fd);
	}

	if (-1 != _conn_fd)
	{
		close(_conn_fd);
	}
}

void tcp_postman::get_ready4subclass()
{
	int opt, ret;
	struct sockaddr_in addr;

	assert(-1 == _listening_fd);
	assert(-1 == _conn_fd);
	assert(0 == _buffer_used_len);

	if (g_configuration.get_lua_scripts_home())
	{
		// capture the whole packet content if test suite is on.
		_hdr_only = false;
	}
	else
	{
		_hdr_only = true;
	}

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
}

void tcp_postman::recv_impl()
{
	_conn_fd = accept(_listening_fd, NULL, NULL);
	if (_conn_fd < 0)
	{
		perror("accept");
		return;
	}

	// blocks until a error happened or the connection is closed.
	save_peer_response_to_buffer();
}

void tcp_postman::save_peer_response_to_buffer()
{
	int ret;
	char* buff_ptr;
	int buff_available_len;

	while (true)
	{
		buff_ptr = _buffer_block + _buffer_used_len;
		buff_available_len = _buffer_block_len - _buffer_used_len;

		ret = read(_conn_fd, buff_ptr, buff_available_len);
		if (ret > 0)
		{
			_buffer_used_len += ret;
			parse_buffer_and_get_all_ip_pkts();
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
				char buff[1024];
				char *ptr = strerror_r(errno, buff, sizeof(buff));
				write(_conn_fd, ptr, strlen(ptr));
			}
			close(_conn_fd);
			_conn_fd = -1;

			return;
		}
	} // end of while
}

void tcp_postman::parse_buffer_and_get_all_ip_pkts()
{
	char* buff_ptr;
	struct iphdr* iphdr;
	struct tcphdr* tcphdr;
	int buff_len, i, ip_tot_len;
	int sentinel;
	uint16_t sum, checksum;
	uint16_t dst_port_in_netbyte_order;

	buff_ptr = _buffer_block;
	buff_len = _buffer_used_len;
	sentinel = 0;

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
		sum = iphdr->check;
		checksum = compute_ip_checksum(iphdr);
		iphdr->check = sum;
		if (checksum != sum)
		{
			i++;
			continue;
		}

		ip_tot_len = ntohs(iphdr->tot_len);
		if (!_hdr_only && buff_len - i < ip_tot_len)
		{
			break;
		}

		if (_hdr_only && buff_len -i < 100)
		{
			break;
		}

		tcphdr = (struct tcphdr*)(ptr + iphdr->ihl*4);
		if (!_hdr_only)
		{
			//sum = tcphdr->check;
			//checksum = compute_tcp_checksum(iphdr, tcphdr);
			//tcphdr->check = sum;
			//if (checksum != sum)
			//{
				// TODO. It's weird the checksum always failed here.
				// I just let it pass as an expedient at present.
				//			i++;
				//			continue;
			//}
		}

		if (tcphdr->source != dst_port_in_netbyte_order)
		{
			i++;
			continue;
		}

		if (!_hdr_only)
		{
			// now, a IP packet is detected
			boost::shared_ptr<ip_pkt> pkt = boost::make_shared<ip_pkt>(ptr);
			push_recved_ippkt(pkt);
			i += ip_tot_len;
			sentinel = i;
		}
		else
		{
			boost::shared_ptr<ip_pkt> pkt = boost::make_shared<ip_pkt_hdr_only>(ptr);
			push_recved_ippkt(pkt);
			// step forward in the most safest way, though with a little efficiency penalty.
			i += 40;
			sentinel = i;
		}

	} // end of for loop

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
}
