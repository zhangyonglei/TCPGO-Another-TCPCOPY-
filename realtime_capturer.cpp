/*********************************************
 * realtime_captureer.cpp
 * Author: kamuszhou@tencent.com 16236914@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 27 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "misc.h"
#include "realtime_capturer.h"
#include "cute_logger.h"
#include "session_manager.h"
#include "proactor.h"

realtime_capturer g_realtime_capturer;

realtime_capturer::realtime_capturer() // : _ippkt_queue(100000)
{
	_traffic_listening_port = 1993;
}

realtime_capturer::~realtime_capturer()
{
}

int realtime_capturer::get_ready()
{
	_count = 0;
	g_proactor.listen(_traffic_listening_port, boost::bind(&realtime_capturer::handle_accept, this, _1, _2));
	_strand.reset(new boost::asio::strand(g_proactor.get_io_service()));

	return 0;
}

void realtime_capturer::inject_realtime_ippkts(int most)
{
	ip_pkt* pkt;
	boost::shared_ptr<ip_pkt> smart_pkt;
	int num = 0;
	while (_ippkt_queue.pop(pkt))
	{
		smart_pkt.reset(pkt);
		g_session_manager.inject_a_realtime_ippkt(smart_pkt);
		_count--;
		num++;
		if (num >= most)
		{
			return;
		}
	}
	// std::cout << num << std::endl;
}

void realtime_capturer::handle_accept(boost::shared_ptr<boost::asio::ip::tcp::socket> s, const boost::system::error_code& error)
{
	using namespace boost::asio;

	uint64_t key = generate_sess_key(s);
	MemPool& mp = _conns[key];

	mp._memblock.reserve(BUFFER_LEN_FOR_TRAFFIC);
	mp._memblock.resize(BUFFER_LEN_FOR_TRAFFIC);
	mp._used_len = 0;

	s->async_read_some(buffer(mp._memblock),
				boost::bind(&realtime_capturer::handle_read, this, s,
						boost::asio::placeholders::error,
						boost::asio::placeholders::bytes_transferred));
}

void realtime_capturer::handle_read(boost::shared_ptr<boost::asio::ip::tcp::socket> s,
									const boost::system::error_code& error,
									std::size_t bytes_transferred)
{
	using namespace boost::asio;

	uint64_t key = generate_sess_key(s);

	// thanks to the shared_ptr, don't have to worry about the fd leak.
	// if ((boost::asio::error::eof == error) ||
	   //     (boost::asio::error::connection_reset == error))
	if (boost::asio::error::eof == error)
	{
		// normal termination
		return;
	}
	else if (error)
	{
	//	throw boost::system::system_error(error);  // some other error.
		return;
	}

	MemPool& mp = _conns[key];
	MemBlock& mb = mp._memblock;
	mp._used_len += bytes_transferred;

	parse_buff_and_get_ip_pkts(mp);

	s->async_read_some(buffer(mb.data() + mp._used_len, mb.size() - mp._used_len),
			_strand->wrap(boost::bind(&realtime_capturer::handle_read, this, s,
							boost::asio::placeholders::error,
							boost::asio::placeholders::bytes_transferred)));
}

uint64_t realtime_capturer::generate_sess_key(boost::shared_ptr<boost::asio::ip::tcp::socket> s)
{
	using namespace boost::asio;

	ip::tcp::socket::endpoint_type point = s->remote_endpoint();
	ip::address_v4 addr = point.address().to_v4();
	unsigned short port = point.port();
	uint64_t key = make_sess_key(addr.to_ulong(), port);

	return key;
}

void realtime_capturer::parse_buff_and_get_ip_pkts(MemPool& mp)
{
	char* buff_ptr;
	struct iphdr* iphdr;
	struct tcphdr* tcphdr;
	int buff_len, i, iphdr_len, ip_tot_len;
	int sentinel;
	uint16_t src_port;
	uint16_t sum, checksum;

	buff_ptr = mp._memblock.data();
	buff_len = mp._used_len;
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
		ip_pkt* pkt = new ip_pkt(ptr);
		while (!_ippkt_queue.push(pkt))
		{
			std::cout << "realtime_capturer's queue is full. _count: " << _count << std::endl;
			if (_count > _ippkt_queue_capacity/15)
			{
				boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
			}
		}
		_count++;
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
		mp._used_len = remaining_data_len;
	}
}
