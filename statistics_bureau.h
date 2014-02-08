/****************************************************************
 * statistics_bureau.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 8 Feb, 2014
 * Happy the Chinese new year of Horse.
 * Today is the first working day after spring festival vacation.
 *****************************************************************/

#ifndef _STATISTICSBUREAU_H_
#define _STATISTICSBUREAU_H_

#include "misc.h"
#include "configuration.h"
#include "thetimer.h"

class statistics_bureau;
extern statistics_bureau g_statistics_bureau;

/**
 * This class records various kinds of statistics.
 */
class statistics_bureau
{
public:
	statistics_bureau();
	virtual ~statistics_bureau();

public:
	std::string sess_statistics();

public:
	void inc_sess_active_close_count()
	{
		_sess_active_close_count++;
		_total_processed_sess_count++;
	}

	void inc_sess_passive_close_count()
	{
		_sess_passive_close_count++;
		_total_processed_sess_count++;
	}

	void inc_sess_cancelled_by_no_response_count()
	{
		_sess_cancelled_by_no_response_count++;
		_total_processed_sess_count++;
	}

	void inc_sess_active_close_timeout_count()
	{
		_sess_active_close_timeout_count++;
		_total_processed_sess_count++;
	}

	void inc_sess_killed_by_reset()
	{
		_sess_killed_by_reset++;
		_total_processed_sess_count++;
	}

private:
	int _total_processed_sess_count;
	int _sess_active_close_count;
	int _sess_passive_close_count;
	int _sess_cancelled_by_no_response_count;
	int _sess_active_close_timeout_count;
	int _sess_killed_by_reset;
};

#endif /* _STATISTICSBUREAU_H_ */
