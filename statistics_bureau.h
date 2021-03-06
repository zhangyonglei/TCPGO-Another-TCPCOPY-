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
class statistics_bureau
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

	void inc_sess_killed_by_reset_count()
	{
		_sess_killed_by_reset_count++;
		_total_processed_sess_count++;
	}

	void inc_sess_dormancy_count()
	{
		_sess_dormancy_count++;
		_total_processed_sess_count++;
	}

	void inc_sess_aborted_count()
	{
		_total_aborted_sess_count++;
	}

	void inc_total_transmit_count()
	{
		_total_transmit_count++;
	}

	void inc_unique_transmit_count()
	{
		_unique_transmit_count++;
	}

	void inc_total_sess_time_duration(uint64_t time)
	{
		_total_sess_time_duration += time;
	}

public:
	void one_shot_timer_event_handler();

private:
	void conn_ends_normally()
	{
		_conns_completed_stats[_current_slot_index]++;
	}

private:
	boost::atomic_int _total_processed_sess_count;
	boost::atomic_int _total_aborted_sess_count;
	boost::atomic_int _sess_active_close_count;
	boost::atomic_int _sess_passive_close_count;
	boost::atomic_int _sess_cancelled_by_no_response_count;
	boost::atomic_int _sess_active_close_timeout_count;
	boost::atomic_int _sess_killed_by_reset_count;
	boost::atomic_int _sess_dormancy_count;
	boost::atomic_int _total_transmit_count;   ///< include retransmit.
	boost::atomic_int _unique_transmit_count;
	boost::atomic_long _total_sess_time_duration;

	// the following variables are intended for make statistics of normally
	// ended connections in the past (_TIME_DURATION_IN_MIN-1) minutes.
	static const int _TIME_DURATION_IN_SECOND = 15 * 60 + 1;
	int _conns_completed_stats[_TIME_DURATION_IN_SECOND];
	int _current_slot_index;

	int _asio_thrd_num;
};

#endif /* _STATISTICSBUREAU_H_ */
