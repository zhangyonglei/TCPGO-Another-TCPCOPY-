/*********************************************
 * horos.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Dec, 2013
 * I'm not a christian. I don't celebrate the christmas day.
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include <pthread.h>
#include "horos.h"
#include "misc.h"
#include "poller.h"

pthread_t             thread;
pthread_attr_t        pta;

extern int run();

void* threadfunc(void *parm)
{
	run();
	return NULL;
}

__attribute__((visibility("default"))) int horos_init()
{
	int ret;

	ret = pthread_create(&thread, NULL, threadfunc, NULL);

	return ret;
}

__attribute__((visibility("default"))) void horos_uninit()
{
	g_poller.stop();
	pthread_join(thread, NULL);
}
