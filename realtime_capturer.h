/*********************************************
 * realtime_captureer.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 27 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _REALTIME_CAPTUREER_H_
#define _REALTIME_CAPTUREER_H_

#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include "misc.h"
#include "utils.h"
#include "ip_pkt.h"

#define CONNECTED_FD_SIZE 32
#define BUFFER_LEN_FOR_TRAFFIC  4096*28

class realtime_capturer;
extern realtime_capturer g_realtime_capturer;

class realtime_capturer
{
public:
	realtime_capturer();

	/**
	 * @return 0 on success, non-zero otherwise.
	 */
	int get_ready();

	void inject_realtime_ippkts(int most);

	virtual ~realtime_capturer();

private:
	/**
	 * the asio accept completion handler.
	 */
	void handle_accept(boost::shared_ptr<boost::asio::ip::tcp::socket> s, const boost::system::error_code& error);

	/**
	 * the asio read commpletion handler.
	 */
	void handle_read(boost::shared_ptr<boost::asio::ip::tcp::socket> s,  const boost::system::error_code& error,
					std::size_t bytes_transferred);

	uint64_t generate_sess_key(boost::shared_ptr<boost::asio::ip::tcp::socket> s);

	struct MemPool
	{
		MemBlock _memblock;
		int _used_len;
	};
	/**
	 * parse the buffer and get all the ip pakckets there
	 * then these pakcets will be removed from buffer.
	 * @param index the connection index.
	 */
	void parse_buff_and_get_ip_pkts(MemPool& mp);

private:
	uint16_t _traffic_listening_port;   ///< in host byte order.

	/// key is created using make_sess_key(ip, port).
	std::map<uint64_t, MemPool>  _conns;
//	boost::lockfree::queue<ip_pkt*> _ippkt_queue;  ///< it's very weird.
	boost::lockfree::spsc_queue<ip_pkt*, boost::lockfree::capacity<50000> > _ippkt_queue;
	int _count;

	boost::shared_ptr<boost::asio::io_service::strand> _strand;   ///< used to serialize the asynchronous I/O
};

#endif /* _REALTIME_CAPTUREER_H_ */
