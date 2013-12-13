/*********************************************
 * cutelogger.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#ifndef _CUTELOGGER_H_
#define _CUTELOGGER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

class cute_logger;
extern cute_logger g_logger;

class cute_logger
{
public:
	cute_logger();
	~cute_logger();

	inline int32_t printf(const char* format, ...);
	inline int32_t flush();

private:
	int32_t _vprintf(const char* format, va_list argptr);

private:
	FILE*      _stream;
	bool       _on;
};

int32_t cute_logger::printf(const char* format, ...)
{
	int32_t ret;
	va_list ap;

	ret = 0;
	if (_on)
	{
		va_start(ap, format);
		ret = vfprintf(_stream, format, ap);
		va_end(ap);
	}

	return ret;
}

int32_t cute_logger::flush()
{
	int32_t ret;

	ret = 0;
	if (_on)
	{
		ret = fflush(_stream);
	}

	return ret;
}



#endif /* _CUTELOGGER_H_ */
