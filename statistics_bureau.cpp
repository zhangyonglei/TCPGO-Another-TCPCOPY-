/*********************************************
 * statistics_bureau.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 8 Feb, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "statistics_bureau.h"
#include "session_manager.h"

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
	_sess_killed_by_reset_count = 0;
	_sess_dormancy_count = 0;

	// if _TIME_DURATION_IN_MIN changed, the corresponding logic has to been modified, too.
	assert(_TIME_DURATION_IN_MIN == 16);
	memset(_conns_completed_stat, 0, sizeof(_conns_completed_stat));
	_current_slot_index = 0;

	g_timer.register_one_shot_timer_event(
			boost::bind(&statistics_bureau::one_shot_timer_event_handler, this), 0);
}

std::string statistics_bureau::sess_statistics()
{
	int response_from_server_time_out;
	int wait_for_fin_from_server_time_out;
	int have_to_send_data_within_this_timeperiod;
	int total_conns_in_15mins;
	int total_conns_in_5mins;
	int total_conns_in_1min;
	int conns_per_second_in_15mins;
	int conns_per_second_in_5mins;
	int conns_per_second_in_1min;
	uint64_t now;
	std::ostringstream ss;

	now = g_timer.get_jiffies();

	response_from_server_time_out = g_configuration.get_response_from_peer_time_out() * (1000 / HZ);
	wait_for_fin_from_server_time_out = g_configuration.get_wait_for_fin_from_peer_time_out() * (1000 / HZ);
	have_to_send_data_within_this_timeperiod = g_configuration.get_have_to_send_data_within_this_timeperiod() * (1000 / HZ);

	total_conns_in_15mins = 0;
	total_conns_in_5mins = 0;
	total_conns_in_1min = 0;
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

	conns_per_second_in_15mins = total_conns_in_15mins/15/60;
	conns_per_second_in_5mins = total_conns_in_5mins/5/60;
	conns_per_second_in_1min = total_conns_in_1min/1/60;

	if (now < 15*60*HZ)
	{
		conns_per_second_in_15mins = conns_per_second_in_5mins;
	}
	if (now < 5*60*HZ)
	{
		conns_per_second_in_5mins = conns_per_second_in_1min;
		conns_per_second_in_15mins = conns_per_second_in_1min;
	}

	ss << g_session_manager.size() << " sessions are now in memory.\n"
	   << _total_processed_sess_count << " sessions have been processed.\n"
	   << _sess_active_close_count << " sessions ended via active close.\n"
	   << _sess_passive_close_count << " sessions ended via passive close.\n"
	   << _sess_cancelled_by_no_response_count << " sessions ended prematurely because of no response from peer within "
	   	   	   	   	   	   	   	   	   	   	   << response_from_server_time_out << " micro seconds.\n"
	   << _sess_active_close_timeout_count << " sessions ended prematurely because sended FIN didn't elicit FIN from server within "
	   	   	   	   	   	   	   	   	   	   	   << wait_for_fin_from_server_time_out << " micro seconds.\n"
	   << _sess_dormancy_count << " sessions ended prematurely because no traffice has been sent to peer within "
	                                           << have_to_send_data_within_this_timeperiod << " micros seconds.\n"
	   << _sess_killed_by_reset_count << " sessions were killed by RESET.\n"
	   << "success rate " << (double)(_sess_passive_close_count + _sess_active_close_count) / (double)_total_processed_sess_count << "\n"

	   << "Up " << now/HZ/60 << " min(s) and " << now/HZ%60 << " second(s).\n";

	if (now/HZ/60 > 0)
	{
		ss << "Average Connections Per Second in the past 15mins, 5mins, and 1min\n"
		   << conns_per_second_in_15mins << " " << conns_per_second_in_5mins << " " << conns_per_second_in_1min;
	}
	else
	{
		ss << "More statistic info will be provided after 1 minute.\n";
	}

	return ss.str();
}

void statistics_bureau::one_shot_timer_event_handler()
{
	_current_slot_index = ++_current_slot_index % _TIME_DURATION_IN_MIN;
	_conns_completed_stat[_current_slot_index] = 0;
	g_timer.register_one_shot_timer_event(
			boost::bind(&statistics_bureau::one_shot_timer_event_handler, this),
			HZ*60);  // triggered in every minute.
}
