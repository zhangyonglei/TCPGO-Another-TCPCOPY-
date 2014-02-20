/*********************************************
 * statistics_bureau.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 8 Feb, 2014
 ********************************************/

#include "statistics_bureau.h"

statistics_bureau g_statistics_bureau;

statistics_bureau::statistics_bureau()
{
}

statistics_bureau::~statistics_bureau()
{
}

void statistics_bureau::get_ready()
{
	_sess_active_close_count = 0;
	_sess_passive_close_count = 0;
	_sess_cancelled_by_no_response_count = 0;
	_sess_active_close_timeout_count = 0;
	_sess_killed_by_reset = 0;

	// if _TIME_DURATION_IN_MIN changed, the corresponding logic has to been modified, too.
	assert(_TIME_DURATION_IN_MIN == 16);
	memset(_conns_completed_stat, 0, sizeof(_conns_completed_stat));
	_current_slot_index = 0;

	g_timer.register_one_shot_timer_event(this, 0);
}

std::string statistics_bureau::sess_statistics()
{
	int response_from_server_time_out;
	int wait_for_fin_from_server_time_out;
	int total_conns_in_15mins = 0;
	int total_conns_in_5mins = 0;
	int total_conns_in_1min = 0;

	std::ostringstream ss;

	response_from_server_time_out = g_configuration.get_response_from_peer_time_out() * (1000 / HZ);
	wait_for_fin_from_server_time_out = g_configuration.get_wait_for_fin_from_peer_time_out() * (1000 / HZ);

	for (int i = 0; i < _TIME_DURATION_IN_MIN - 1; i++)
	{
		int idx = (_current_slot_index + i + 1) % _TIME_DURATION_IN_MIN;
		int count = _conns_completed_stat[idx];

		total_conns_in_15mins += count;
		if ( i >= 10 )
		{
			total_conns_in_5mins += count;
		}
		if ( i >= 14)
		{
			total_conns_in_1min += count;
		}
	}

	ss << _total_processed_sess_count << " sessions have been processed.\n"
	   << _sess_active_close_count << " sessions ended via active close.\n"
	   << _sess_passive_close_count << " sessions ended via passive close.\n"
	   << _sess_cancelled_by_no_response_count << " sessions ended prematurely because of no response within "
	   	   	   	   	   	   	   	   	   	   	   << response_from_server_time_out << " micro seconds.\n"
	   << _sess_active_close_timeout_count << " sessions ended prematurely because sended FIN didn't elicit FIN from server within "
	   	   	   	   	   	   	   	   	   	   	   << wait_for_fin_from_server_time_out << " micro seconds.\n"
	   << _sess_killed_by_reset << " sessions were killed by RESET.\n"
	   << "success rate " << (double)(_sess_passive_close_count + _sess_active_close_count) / (double)_total_processed_sess_count << ""

	   << "Connections Per Second in the past 15mins, 10mins, and 1min\n"
	   << total_conns_in_15mins/15/60 << " " << total_conns_in_5mins/5/60 << " " << total_conns_in_1min/1/60;

	return ss.str();
}

void statistics_bureau::one_shot_timer_event_run()
{
	_current_slot_index = ++_current_slot_index % _TIME_DURATION_IN_MIN;
	_conns_completed_stat[_current_slot_index] = 0;
	g_timer.register_one_shot_timer_event(this, HZ*60);  // triggered in every minute.
}
