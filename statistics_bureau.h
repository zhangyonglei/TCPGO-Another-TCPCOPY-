/****************************************************************
 * statistics_bureau.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 8 Feb, 2014
 * Happy the Chinese new year of Horse.
 * Today is the first working day after spring festival vacation.
 * Praise Be to the Lord. BUG-FREE CODE !
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
class statistics_bureau : public timer_event
{
public:
	statistics_bureau();
	virtual ~statistics_bureau();

public:
	std::string sess_statistics();

public:
	void get_ready();

	void inc_sess_active_close_count()
	{
		_sess_active_close_count++;
		_total_processed_sess_count++;

		conn_ends_normally();
	}

	void inc_sess_passive_close_count()
	{
		_sess_passive_close_count++;
		_total_processed_sess_count++;

		conn_ends_normally();
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

public:
	void one_shot_timer_event_run();

private:
	void conn_ends_normally()
	{
		_conns_completed_stat[_current_slot_index]++;
	}

private:
	int _total_processed_sess_count;
	int _sess_active_close_count;
	int _sess_passive_close_count;
	int _sess_cancelled_by_no_response_count;
	int _sess_active_close_timeout_count;
	int _sess_killed_by_reset;

	// the following variables are intended for make statistics of normally
	// ended connections in the past (_TIME_DURATION_IN_MIN-1) minutes.
	static const int _TIME_DURATION_IN_MIN = 16;
	int _conns_completed_stat[_TIME_DURATION_IN_MIN];
	int _current_slot_index;
};

#endif /* _STATISTICSBUREAU_H_ */
