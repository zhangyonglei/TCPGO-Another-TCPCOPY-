/*********************************************
 * configuration.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 16 Jan, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
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

public:
	enum SNIFF_METHOD{SNIFF_RAW, SNIFF_PCAP, SNIFF_TCP};

	/**
	 * @return return true if all parameters are legitimate, otherwise false.
	 */
	bool check_validity();

	// getters
	std::string get_conf_file_path()
	{
		return _conf_file_path;
	}

	// [MAIN]
	std::string get_pcap_file_path()
	{
		return _pcap_file_path;
	}

	std::string get_dst_addr()
	{
		return _dst_addr;
	}

	// host-byte order.
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

	int get_accidental_death_pcap_file_limit()
	{
		return _accidental_death_pcap_file_limit;
	}

	SNIFF_METHOD get_sniff_method()
	{
		return _sniff_method;
	}

	std::string get_next_hop()
	{
		return _next_hop;
	}

	// [SESSION]
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

	bool get_enable_active_close()
	{
		return _enable_active_close;
	}

	// [LOG]
	bool get_log_on()
	{
		return _log_on;
	}

	bool get_duplicate_log_to_stdout()
	{
		return _duplicate_log_to_stdout;
	}

	// [TESTSUITE]
	const std::string& get_lua_scripts_home()
	{
		return _lua_scripts_home;
	}

	const std::string get_so_home()
	{
		return _so_home;
	}

	// setters
	void set_conf_file_path(const std::string& conf_file_path);

	// [MAIN]
	void set_pcap_file_path(const std::string& pcap_file_path);

	void set_dst_addr(const std::string& dst_addr);

	void set_dst_port(const std::string& dst_port);
	void set_dst_port(unsigned short dst_port);

	void set_concurrency_limit(const std::string& concurrency_limit);
	void set_concurrency_limit(int concurrency_limit);

	void set_accidental_death_pcap_file_limit(const std::string& limit);
	void set_accidental_death_pcap_file_limit(int limit);

	void set_sniff_method(const std::string& sniff_method);
	void set_sniff_method(SNIFF_METHOD sniff_method);

	void set_next_hop(const std::string& next_hop);

	// [SESSION]
	void set_onoff_random_port(const std::string& onoff_random_port);
	void set_onoff_random_port(bool onoff_random_port);

	void set_response_from_peer_time_out(const std::string& response_from_peer_time_out);
	void set_response_from_peer_time_out(int response_from_peer_time_out);

	void set_have_to_send_data_within_this_timeperiod(const std::string& timeperiod);
	void set_have_to_send_data_within_this_timeperiod(int timeperiod);

	void set_snd_speed_control(const std::string& speed_control);
	void set_snd_speed_control(int speed_control);

	void set_wait_for_fin_from_peer_time_out(const std::string& time_out);
	void set_wait_for_fin_from_peer_time_out(int time_out);

	void set_enable_active_close(const std::string& enable);
	void set_enable_active_close(bool enable);

	// [LOG]
	void set_duplicate_log_to_stdout(const std::string& dup);
	void set_duplicate_log_to_stdout(bool dup);

	void set_log_on(const std::string& log_on);
	void set_log_on(bool log_on);

	// [TESTSUITE]
	void set_lua_scripts_home(const std::string& home);
	void set_so_home(const std::string& home);

	/**
	 * read in conf file.
	 */
	void readin();

private:
	std::string  _conf_file_path;   ///< self-explanatory.

	// [MAIN]
	std::string  _pcap_file_path;   ///< load traffic from the pcap file.
	std::string  _dst_addr;         ///< the desination IP address to which traffic will be forward.
	unsigned short _dst_port;   ///< the destination port to which traffic will be forward. host-byte order.
	int _concurrency_limit;     ///< the maximum number of concurrent TCP connections.
	bool _onoff_random_port; ///< specify if the source port of TCP segment will be replaced with a random number.
	int _accidental_death_pcap_file_limit; ///< at least how many accidental death corpses will be saved as pcap file on disk.
	SNIFF_METHOD _sniff_method; ///< there are three kinds of methods to sniff the peers response packets.
	std::string _next_hop;   ///< ONLY available when _sniff_method is SNIFF_TCP. used to changed together all the running horos instances.

	// [SESSION]
	/// The following parameters are for tuning tcpsession.
	/// More details can be found in class tcpsession.
	/// They are all in unit of centisecond.
	int  _response_from_peer_time_out;
	int  _have_to_send_data_within_this_timeperiod;
	int  _snd_speed_control;
	int  _wait_for_fin_from_peer_time_out;
	bool _enable_active_close;

	// [LOG]
	bool _log_on;
	bool _duplicate_log_to_stdout;

	// [TESTSUITE]
	std::string _lua_scripts_home;
	std::string _so_home;
};

#endif /* _CONFIGURATION_H_ */
