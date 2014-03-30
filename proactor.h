/*********************************************
 * proactor.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 28 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _PROACTOR_H_
#define _PROACTOR_H_

#include "misc.h"
#include <boost/asio.hpp>

class proactor;
extern proactor g_proactor;

typedef boost::function<void (boost::shared_ptr<boost::asio::ip::tcp::socket>,
		const boost::system::error_code& error)> acceptor_handler;

class proactor
{
public:
	proactor();

	void get_ready();

	virtual ~proactor();

	/**
	 * produce a timer.
	 */
	boost::shared_ptr<boost::asio::deadline_timer>  produce_a_timer(boost::posix_time::time_duration td);

	/**
	 * open a tcp listening port on all the interface.
	 * @param port the port which is listened on.
	 * @param handler called when the accept() system call finished.
	 */
	void listen(uint16_t port, acceptor_handler handler);

	boost::asio::io_service& get_io_service()
	{
		return _io_service;
	}

private:
	void handle_accept_proxy(boost::shared_ptr<boost::asio::ip::tcp::acceptor> a,
			boost::shared_ptr<boost::asio::ip::tcp::socket> s,
			acceptor_handler h,
			const boost::system::error_code& error);

private:
	boost::asio::io_service _io_service;
	boost::asio::io_service::work _work;

	int _threads_count;
	boost::thread_group _threads;

	typedef boost::asio::ip::tcp::acceptor the_acceptor;
	std::map<uint16_t, boost::shared_ptr<boost::asio::ip::tcp::acceptor> > _listeners;
};

#endif /* _PROACTOR_H_ */
