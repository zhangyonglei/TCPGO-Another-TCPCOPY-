/*********************************************
 * thetimer.cpp
 * Author: kamuszhou@tencent.com,kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: 16 Dec, 2013
 ********************************************/

#include "misc.h"
#include "thetimer.h"

the_timer g_timer;

void sig_alarm_handler(int sig)
{
	g_timer._jiffies++;
	//std::cout << g_timer._jiffles << std::endl;
}

the_timer::the_timer()
{
    signal(SIGALRM, sig_alarm_handler);
    _jiffies = 0;
}

the_timer::~the_timer()
{
}

void the_timer::get_ready()
{
    struct itimerval value;
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
    }
}

void the_timer::register_one_shot_timer_event(timer_event* evt, uint32_t relative_time)
{
	uint64_t now, fire_time;

	now = _jiffies;
	fire_time = now + relative_time;
	_tmp_events.insert(std::pair<uint64_t, timer_event*>(fire_time, evt));
}

void the_timer::loop_through_all_timer_event()
{
	std::multimap<uint64_t, timer_event*>::iterator ite;
	_events.insert(_tmp_events.begin(), _tmp_events.end());
	_tmp_events.clear();

	for (ite = _events.begin(); ite != _events.end(); )
	{
		// fire it !
		if (ite->first <= _jiffies)
		{
			ite->second->one_shot_timer_event_run();
			_events.erase(ite++);
		}
		else
		{
			++ite;
		}
	}
}
