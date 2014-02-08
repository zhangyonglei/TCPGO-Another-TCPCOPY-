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
	_sess_active_close_count = 0;
	_sess_passive_close_count = 0;
	_sess_cancelled_by_no_response_count = 0;
	_sess_active_close_timeout_count = 0;
	_sess_killed_by_reset = 0;
}

statistics_bureau::~statistics_bureau()
{
}

std::string statistics_bureau::sess_statistics()
{
	int response_from_server_time_out;
	int wait_for_fin_from_server_time_out;
	std::ostringstream ss;

	response_from_server_time_out = g_configuration.get_response_from_peer_time_out() * (1000 / HZ);
	wait_for_fin_from_server_time_out = g_configuration.get_wait_for_fin_from_peer_time_out() * (1000 / HZ);

	ss << _total_processed_sess_count << " sessions have been processed.\n"
	   << _sess_active_close_count << " sessions ended via active close.\n"
	   << _sess_passive_close_count << " sessions ended via passive close.\n"
	   << _sess_cancelled_by_no_response_count << " sessions ended prematurely because of no response within "
	   	   	   	   	   	   	   	   	   	   	   << response_from_server_time_out << " micro seconds.\n"
	   << _sess_active_close_timeout_count << " sessions ended prematurely because sended FIN didn't elicit FIN from server with "
	   	   	   	   	   	   	   	   	   	   	   << wait_for_fin_from_server_time_out << " micro seconds.\n"
	   << _sess_killed_by_reset << " sessions were killed by RESET.\n";

	return ss.str();
}
