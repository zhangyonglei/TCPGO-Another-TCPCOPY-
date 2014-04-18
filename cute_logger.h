/*********************************************
 * cutelogger.h
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _CUTELOGGER_H_
#define _CUTELOGGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>
#include "configuration.h"

class cute_logger;
extern cute_logger g_logger;

class cute_logger
{
public:
	cute_logger();
	~cute_logger();

	inline int printf(const char* format, ...);
	inline int flush();

private:
	int _vprintf(const char* format, va_list argptr);

private:
	FILE*  _stream;
};

int cute_logger::printf(const char* format, ...)
{
	int ret;
	va_list ap;

	ret = 0;
	if (g_configuration.get_log_on())
	{
		va_start(ap, format);
		ret = vfprintf(_stream, format, ap);
		va_end(ap);
	}

	if (g_configuration.get_duplicate_log_to_stdout())
	{
		va_start(ap, format);
//		vsnprintf(buff, sizeof(buff), format, ap);
		vprintf(format, ap);
		va_end(ap);
		// it's assumed if log is duplicated on the screen, then you are expecting to flush
		// all logs into disk file ASAP to provide more log details for debug.
		// for the sake of efficiency, don't duplicate logs to screen when it's running on production condition.
		fflush(_stream);
	}

	return ret;
}

int cute_logger::flush()
{
	int ret;

	ret = 0;
	if (g_configuration.get_log_on())
	{
		ret = fflush(_stream);
	}

	return ret;
}

#endif /* _CUTELOGGER_H_ */
