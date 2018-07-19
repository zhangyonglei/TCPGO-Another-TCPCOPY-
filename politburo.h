/*********************************************
 * politburo.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  http://blog.ykyi.net
 * Created on: 30 Mar, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _POLITBURO_H_
#define _POLITBURO_H_

#include <boost/asio/deadline_timer.hpp>
#include "session_manager.h"
#include "postoffice.h"

class politburo;
extern politburo g_politburo;

typedef boost::function<void (const boost::system::error_code& error)> WaitHandler;

/**
 * politburo is the center of the power.
 */
class politburo
{
public:
	politburo();
	virtual ~politburo();

	void get_ready();

	int read_from_pcapfile(const std::string& path, const std::string& filter);

	boost::shared_ptr<boost::asio::deadline_timer> enqueue_a_timer_handler(int asio_idx,
								boost::posix_time::time_duration td, WaitHandler handler);

private:
	void post_the_jobs(int asio_idx);

private:
	std::vector<boost::shared_ptr<boost::asio::io_service::strand> > _strands;
	int _asio_thrd_num;
};

#endif /* _POLITBURO_H_ */
