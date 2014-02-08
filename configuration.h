/*********************************************
 * configuration.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 16 Jan, 2014
 ********************************************/

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <string>
#include "INIParser.h"

class configuration;
extern configuration g_configuration;

class configuration
{
public:
	configuration();
	virtual ~configuration();

	/**
	 * @return return true if all parameters are legitimate, otherwise false.
	 */
	bool check_validity();

	// getters
	std::string get_conf_file_path()
	{
		return _conf_file_path;
	}

	std::string get_pcap_file_path()
	{
		return _pcap_file_path;
	}

	std::string get_dst_addr()
	{
		return _dst_addr;
	}

	unsigned short get_dst_port()
	{
		return _dst_port;
	}

	int get_concurrency_limit()
	{
		return _concurrency_limit;
	}

	bool get_onoff_random_port()
	{
		return _onoff_random_port;
	}

	int get_response_from_peer_time_out()
	{
		return _response_from_peer_time_out;
	}

	int get_have_to_send_data_within_this_timeperiod()
	{
		return _have_to_send_data_within_this_timeperiod;
	}

	int get_snd_speed_control()
	{
		return _snd_speed_control;
	}

	int get_wait_for_fin_from_peer_time_out()
	{
		return _wait_for_fin_from_peer_time_out;
	}

	// setters
	void set_conf_file_path(const std::string& conf_file_path);

	void set_pcap_file_path(const std::string& pcap_file_path);

	void set_dst_addr(const std::string& dst_addr);

	void set_dst_port(const std::string& dst_port);

	void set_concurrency_limit(const std::string& concurrency_limit);

	void set_onoff_random_port(const std::string& onoff_random_port);

	void set_response_from_peer_time_out(const std::string& response_from_peer_time_out);
	void set_response_from_peer_time_out(int response_from_peer_time_out);

	void set_have_to_send_data_within_this_timeperiod(const std::string& timeperiod);
	void set_have_to_send_data_within_this_timeperiod(int timeperiod);

	void set_snd_speed_control(const std::string& speed_control);
	void set_snd_speed_control(int speed_control);

	void set_wait_for_fin_from_peer_time_out(const std::string& time_out);
	void set_wait_for_fin_from_peer_time_out(int time_out);

	/**
	 * read in conf file.
	 */
	void readin();

private:
	std::string  _conf_file_path;   ///< self-explanatory.

	std::string  _pcap_file_path;   ///< load traffic from the pcap file.

	std::string  _dst_addr;         ///< the desination IP address to which traffic will be forward.

	unsigned short _dst_port;   ///< the destination port to which traffic will be forward. host-byte order.

	int _concurrency_limit;     ///< the maximum number of concurrent TCP connections.

	bool _onoff_random_port; ///< specify if the source port of TCP segment will be replaced with a random number.

	/// The following parameters are for tuning tcpsession.
	/// More details can be found in class tcpsession.
	/// They are all in unit of centisecond.
	int  _response_from_peer_time_out;
	int  _have_to_send_data_within_this_timeperiod;
	int  _snd_speed_control;
	int  _wait_for_fin_from_peer_time_out;
};

#endif /* _CONFIGURATION_H_ */
