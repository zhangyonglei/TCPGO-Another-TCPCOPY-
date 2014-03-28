/*********************************************
 * proactor.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 28 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include <boost/enable_shared_from_this.hpp>
#include "proactor.h"

proactor g_proactor;

class proactor_exception :
	public boost::exception,
	public std::exception
{
public:
	proactor_exception(const std::string& hint) : _what(hint)
	{
	}

	// throw() guarantees that no exception will be thrown.
	virtual const char* what() const throw()
	{
		return _what.c_str();
	}

	~proactor_exception()throw()
	{
	}

private:
	std::string _what;
};

proactor::proactor() : _work(_io_service)
{
}

proactor::~proactor()
{
	_io_service.stop();
	_threads.join_all();
}

void proactor::get_ready()
{
	for (int i = 0; i < _threads_count; i++)
	{
		_threads.create_thread(boost::bind(&boost::asio::io_service::run, &_io_service));
	}
}

boost::shared_ptr<boost::asio::deadline_timer> proactor::produce_a_timer(boost::posix_time::time_duration td)
{
	return boost::make_shared<boost::asio::deadline_timer>(boost::ref(_io_service), boost::ref(td));
}

void proactor::listen(uint16_t port, acceptor_handler handler)
{
	using boost::asio::ip::tcp;

	if (_listeners.find(port) != _listeners.end())
	{
		std::ostringstream ss;
		ss << port << " has already been occupied.\n";
		throw proactor_exception(ss.str());
	}

	// construct an acceptor opened on the given endpoint.
	boost::shared_ptr<the_acceptor> a;
	a.reset(
			new the_acceptor(_io_service, tcp::endpoint(tcp::v4(), port))
	);

	// start the async accept.
	boost::shared_ptr<tcp::socket> s = boost::make_shared<tcp::socket>(boost::ref(_io_service));
	a->async_accept(*s, boost::bind(handler, s, boost::asio::placeholders::error));

	_listeners[port] = a;
}
