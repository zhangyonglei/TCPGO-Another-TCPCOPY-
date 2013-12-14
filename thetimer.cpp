/*********************************************
 * thetimer.cpp
 * Author: kamuszhou@tencent.com,16236914@qq.com
 * website: www.dogeye.net
 * Created on: 16 Dec, 2013
 ********************************************/

#include "misc.h"
#include "thetimer.h"

the_timer g_timer;

void sig_alarm_handler(int sig)
{
	g_timer._jiffles++;
	std::cout << g_timer._jiffles << std::endl;
}

the_timer::the_timer()
{
    signal(SIGALRM, sig_alarm_handler);
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
