/*********************************************
 * thetimer.h
 * Author: kamuszhou@tencent.com,kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: 16 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _THETIMER_H_
#define _THETIMER_H_

#include "misc.h"

#define HZ 100

class the_timer;
extern the_timer g_timer;

typedef boost::function<void (void)> timer_event_handler;

class the_timer
{
public:
	the_timer();
	virtual ~the_timer();

	void get_ready();

	uint64_t get_jiffies()
	{
		return _jiffies;
	}

	/**
	 * This function returns the ID of the one-shot-timer.
	 */
	uint64_t register_one_shot_timer_event(timer_event_handler evt_hdl, uint32_t relative_time);

	/**
	 * return true if the timer was found and removed.
	 * return false if the timer doesn't exist.
	 */
	bool remove_the_timer(uint64_t timer_id);

	void loop_through_all_timer_event();

public:
	void timer_handler();

private:
	// obsoleted. Use the boost style timer instead.
	// 'cos 2.6.10 kernel doesn't support epoll_pwait.
	// friend void sig_alarm_handler(int sig);

private:
	/// "jiffles" is a buzzword in linux kernel. Here, much like in kernel sourcecode,
	/// Variable _jiffies holds the number of ticks that have occurred since the program
	/// startup. On startup, the kernel initializes the variable to zero, and it is incremented by
	/// one during each timer interrupt.Thus, because there are HZ timer interrupts in a second,
	/// there are HZ jiffies in a second.The system uptime is therefore jiffies/HZ seconds.What
	volatile uint64_t  _jiffies;
	uint64_t _next_available_id;
	typedef std::pair<uint64_t, timer_event_handler> EventElement;
	std::multimap<uint64_t, EventElement> _events;     ///< key is the ID of the timer.
	std::multimap<uint64_t, EventElement> _tmp_events; ///< to evade iterator invalidation.

	boost::shared_ptr<boost::asio::deadline_timer> _deadline_timer;

	boost::atomic<bool> _done;
};

#endif /* _THETIMER_H_ */
