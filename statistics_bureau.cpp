/*********************************************
 * statistics_bureau.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 8 Feb, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "statistics_bureau.h"
#include "session_manager.h"
#include "realtime_capturer.h"

statistics_bureau g_statistics_bureau;

statistics_bureau::statistics_bureau()
{
}

statistics_bureau::~statistics_bureau()
{
}

void statistics_bureau::get_ready()
{
	_total_processed_sess_count = 0;
	_total_aborted_sess_count = 0;
	_sess_active_close_count = 0;
	_sess_passive_close_count = 0;
	_sess_cancelled_by_no_response_count = 0;
	_sess_active_close_timeout_count = 0;
	_sess_killed_by_reset_count = 0;
	_sess_dormancy_count = 0;
	_total_transmit_count = 0;
	_unique_transmit_count = 0;
	_total_sess_time_duration = 0;

	_asio_thrd_num = g_configuration.get_asio_thrd_num();

	// if _TIME_DURATION_IN_MIN changed, the corresponding logic has to been modified, too.
	memset(_conns_completed_stats, 0, sizeof(_conns_completed_stats));
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
	int total_conns_in_15seconds;
	int total_conns_in_5seconds;
	int total_conns_in_1second;
	int conns_per_second_in_15mins;
	int conns_per_second_in_5mins;
	int conns_per_second_in_1min;
	int conns_per_second_in_15seconds;
	int conns_per_second_in_5seconds;
	int conns_per_second_in_1second;
	uint64_t now;
	std::ostringstream ss;

	now = g_timer.get_jiffies();

	response_from_server_time_out = g_configuration.get_response_from_peer_time_out() * (1000 / HZ);
	wait_for_fin_from_server_time_out = g_configuration.get_wait_for_fin_from_peer_time_out() * (1000 / HZ);
	have_to_send_data_within_this_timeperiod = g_configuration.get_have_to_send_data_within_this_timeperiod() * (1000 / HZ);

	total_conns_in_15mins = 0;
	total_conns_in_5mins = 0;
	total_conns_in_1min = 0;
	total_conns_in_15seconds = 0;
	total_conns_in_5seconds = 0;
	total_conns_in_1second = 0;
	for (int i = 1; i <= _TIME_DURATION_IN_SECOND - 1; i++)
	{
		int idx = (_current_slot_index + i) % _TIME_DURATION_IN_SECOND;
		int count = _conns_completed_stats[idx];

		total_conns_in_15mins += count;

		if (i > 10 * 60)
		{
			total_conns_in_5mins += count;
		}

		if (i > 14 * 60)
		{
			total_conns_in_1min += count;
		}

		if (i > 14 * 60 + 45)
		{
			total_conns_in_15seconds += count;
		}

		if (i > 14 * 60 + 55)
		{
			total_conns_in_5seconds += count;
		}

		if (i == 15 * 60)
		{
			total_conns_in_1second += count;
		}
	}

	conns_per_second_in_15mins = total_conns_in_15mins/15/60;
	conns_per_second_in_5mins = total_conns_in_5mins/5/60;
	conns_per_second_in_1min = total_conns_in_1min/1/60;
	conns_per_second_in_15seconds = total_conns_in_15seconds/15;
	conns_per_second_in_5seconds = total_conns_in_5seconds/5;
	conns_per_second_in_1second = total_conns_in_1second/1;

	if (now < 15*60*HZ)
	{
		conns_per_second_in_15mins = -1;
	}
	if (now < 5*60*HZ)
	{
		conns_per_second_in_5mins = -1;
	}
	if (now < 1*60*HZ)
	{
		conns_per_second_in_1min = -1;
	}
	if (now < 15*HZ)
	{
		conns_per_second_in_15seconds = -1;
	}
	if (now < 5*HZ)
	{
		conns_per_second_in_5seconds = -1;
	}
	if (now < HZ)
	{
		conns_per_second_in_1second = -1;
	}

	int total_count = 0;
	int healthy_sess_count = 0;
	for (int i = 0; i < _asio_thrd_num; i++)
	{
		total_count += session_manager::instance(i).size();
		healthy_sess_count += session_manager::instance(i).get_healthy_sess_count();
	}

	ss << std::setprecision(4) << std::fixed;

	ss << total_count << " sessions are now in memory.\n"
	   << healthy_sess_count << " sessions are healthy.\n";

	if (g_realtime_capturer.is_in_traffic_jam_control())
	{
		ss << "In provisional traffic jam control.\n";
	}

	ss << _total_processed_sess_count << " sessions have been processed.(excluding aborted sessions)\n"
	   << _total_aborted_sess_count << " sessions have aborted.\n"
	   << _sess_active_close_count << " sessions ended via active close.\n"
	   << _sess_passive_close_count << " sessions ended via passive close.\n"
	   << _sess_cancelled_by_no_response_count << " sessions ended prematurely because of no response from peer within "
	   	   	   	   	   	   	   	   	   	   	   << response_from_server_time_out * 10 << " milliseconds.\n"
	   << _sess_active_close_timeout_count << " sessions ended prematurely because sended FIN didn't elicit FIN from server within "
	   	   	   	   	   	   	   	   	   	   	   << wait_for_fin_from_server_time_out * 10 << " milliseconds.\n"
	   << _sess_dormancy_count << " sessions ended prematurely because no traffice has been sent to peer within "
	                                           << have_to_send_data_within_this_timeperiod * 10 << " milliseconds.\n"
	   << _sess_killed_by_reset_count << " sessions were killed by RESET.\n"
	   << "Average Session Time Duration: " << (double)_total_sess_time_duration * 10 / (double)(_sess_passive_close_count + _sess_active_close_count) << " milliseconds.\n"
	   << "Retransmit Rate: " << (double)(_total_transmit_count - _unique_transmit_count) / (double)_unique_transmit_count << "\n"
	   << "Success Rate: " << (double)(_sess_passive_close_count + _sess_active_close_count) / (double)_total_processed_sess_count << "\n"

	   << "Up " << now/HZ/60 << " min(s) and " << now/HZ%60 << " second(s).\n";

	ss << "Average Connections Per Second in the past 15mins, 5mins, 1min, 15seconds, 5seconds, 1second:\n"
	   << conns_per_second_in_15mins << " " << conns_per_second_in_5mins << " " << conns_per_second_in_1min << " "
	   << conns_per_second_in_15seconds << " " << conns_per_second_in_5seconds << " " << conns_per_second_in_1second << " "
	   << " (active_closed + passive_closed) / time elapsed in second)";

	return ss.str();
}

void statistics_bureau::one_shot_timer_event_handler()
{
	_current_slot_index = ++_current_slot_index % _TIME_DURATION_IN_SECOND;
	_conns_completed_stats[_current_slot_index] = 0;
	g_timer.register_one_shot_timer_event(
			boost::bind(&statistics_bureau::one_shot_timer_event_handler, this),
			HZ);  // triggered in every second.
}
