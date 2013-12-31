/*********************************************
 * cutelogger.cpp
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#include "cute_logger.h"

cute_logger g_logger;

cute_logger::cute_logger()
{
	_stream = fopen("h.log", "w+");
	if (NULL == _stream)
		abort();
	_on = true;
}

cute_logger::~cute_logger()
{
	fclose(_stream);
}
