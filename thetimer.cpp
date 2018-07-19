/*********************************************
 * thetimer.cpp
 * Author: kamuszhou@tencent.com,kamuszhou@qq.com
 * website: http://blog.ykyi.net
 * Created on: 16 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "misc.h"
#include "thetimer.h"
#include "proactor.h"

the_timer g_timer;

/* Obsoleted, use the boost way instead. Refer the the_timer::timer_handler.
void sig_alarm_handler(int sig)
{
	g_timer._jiffies++;
//	std::cout << g_timer._jiffies << std::endl;
}*/

the_timer::the_timer()
{
//    signal(SIGALRM, sig_alarm_handler);
    _jiffies = 0;
}

the_timer::~the_timer()
{
	_done = true;
}

void the_timer::get_ready()
{
	// use the boost way instead.
 /*   struct itimerval value;
    int msec;

    msec = 1000 / HZ;

    value.it_value.tv_sec = msec / 1000;
    value.it_value.tv_usec = (msec % 1000) * 1000;
    value.it_interval.tv_sec = msec / 1000;
    value.it_interval.tv_usec = (msec % 1000) * 1000;

    if (setitimer(ITIMER_REAL, &value, NULL) == -1)
    {
    	perror("setitimer: ");
    	abort();
    }*/

	_next_available_id = 1;
	_done = false;

	boost::posix_time::time_duration td = boost::posix_time::milliseconds(10);
	_deadline_timer = g_proactor.produce_a_timer(td);
	_deadline_timer->async_wait(boost::bind(&the_timer::timer_handler, this));
}

void the_timer::timer_handler()
{
	if (_done)
		return;

	_jiffies++;
//	std::cout << _jiffies << std::endl;

	_deadline_timer->expires_at(_deadline_timer->expires_at() + boost::posix_time::milliseconds(10));
	_deadline_timer->async_wait(boost::bind(&the_timer::timer_handler, this));
}

uint64_t the_timer::register_one_shot_timer_event(timer_event_handler evt_hdl, uint32_t relative_time)
{
	uint64_t now, fire_time;

	now = _jiffies;
	fire_time = now + relative_time;
	_tmp_events.insert(std::pair<uint64_t, EventElement>(
			_next_available_id, EventElement(fire_time, evt_hdl)
			));

	return _next_available_id++;
}

bool the_timer::remove_the_timer(uint64_t timer_id)
{
	int ret;
	ret = _tmp_events.erase(timer_id);

	if (0 == ret)
	{
		ret = _events.erase(timer_id);
	}

	return ret;
}

void the_timer::loop_through_all_timer_event()
{
	std::multimap<uint64_t, EventElement>::iterator ite;
	_events.insert(_tmp_events.begin(), _tmp_events.end());
	_tmp_events.clear();

	for (ite = _events.begin(); ite != _events.end(); )
	{
		// fire it !
		if (ite->second.first <= _jiffies)
		{
			ite->second.second();
			_events.erase(ite++);
		}
		else
		{
			++ite;
		}
	}
}
