/*********************************************
 * thetimer.h
 * Author: kamuszhou@tencent.com,kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: 16 Dec, 2013
 ********************************************/

#ifndef _THETIMER_H_
#define _THETIMER_H_

#include <map>

#define HZ 100

class the_timer;
extern the_timer g_timer;

class timer_event
{
public:
	virtual void one_shot_timer_event_run() = 0;
	virtual ~timer_event(){};
};

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

	void register_one_shot_timer_event(timer_event* evt, uint32_t relative_time);

	void loop_through_all_timer_event();

private:
	friend void sig_alarm_handler(int sig);

private:
	/// "jiffles" is a buzzword in linux kernel. Here, much like in kernel sourcecode,
	// Variable _jiffies holds the number of ticks that have occurred since the program
	// startup. On startup, the kernel initializes the variable to zero, and it is incremented by
	// one during each timer interrupt.Thus, because there are HZ timer interrupts in a second,
	// there are HZ jiffies in a second.The system uptime is therefore jiffies/HZ seconds.What
	volatile uint64_t  _jiffies;
	std::multimap<uint64_t, timer_event*> _events;
	std::multimap<uint64_t, timer_event*> _tmp_events; ///< to evade iterator invalidation.
};

#endif /* _THETIMER_H_ */
