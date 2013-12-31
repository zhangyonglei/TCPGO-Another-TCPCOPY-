/*********************************************
 * realtime_captureer.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 27 Dec, 2013
 ********************************************/

#ifndef _REALTIME_CAPTUREER_H_
#define _REALTIME_CAPTUREER_H_

#include "misc.h"
#include "utils.h"
#include "poller.h"

#define CONNECTED_FD_SIZE 32
#define BUFFER_LEN_FOR_TRAFFIC  4096*5

class realtime_captureer;
extern realtime_captureer g_realtime_captureer;

class realtime_captureer : public evt_workhorse
{
public:
	realtime_captureer();

	/**
	 * @return 0 on success, non-zero otherwise.
	 */
	int get_ready();

	virtual void pollin_handler(int fd);
	virtual void pollout_handler(int fd);

	virtual ~realtime_captureer();

private:
	void accept_conn();
	void readin_traffic(int fd);
	void close_fd(int fd);
	int map_fd_to_index(int fd);

	/**
	 * parse the buffer and get all the ip pakckets there
	 * then these pakcets will be removed from buffer.
	 * @param index the connection index.
	 */
	void parse_buff_and_get_ip_pkts(int index);

private:
	uint16_t _listening_port;   ///< in host byte order.
	int _listening_fd;

	struct conn_info
	{
		int _fd;
		char* _buffer_block;
		int _buffer_used_len;
	}_conns[CONNECTED_FD_SIZE];
};

#endif /* _REALTIME_CAPTUREER_H_ */
