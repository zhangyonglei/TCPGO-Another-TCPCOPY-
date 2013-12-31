/*********************************************
 * cutelogger.h
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#ifndef _CUTELOGGER_H_
#define _CUTELOGGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <iostream>

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
	FILE*      _stream;
	bool       _on;
};

int cute_logger::printf(const char* format, ...)
{
	int ret;
	va_list ap;
	char buff[2048] = { 0 };

	ret = 0;
	if (_on)
	{
		va_start(ap, format);
		ret = vfprintf(_stream, format, ap);
		va_end(ap);
	}

#ifdef __DEBUG__
	va_start(ap, format);
	vsnprintf(buff, sizeof(buff), format, ap);
	va_end(ap);
	std::cout << buff;
	fflush(_stream);
#endif

	return ret;
}

int cute_logger::flush()
{
	int ret;

	ret = 0;
	if (_on)
	{
		ret = fflush(_stream);
	}

	return ret;
}



#endif /* _CUTELOGGER_H_ */
