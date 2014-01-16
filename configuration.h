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
		return _dst_port_num;
	}

	int get_concurrency_limit()
	{
		return _concurrency_limit_num;
	}

	bool get_onoff_random_port()
	{
		return _onoff_random_port_boolean;
	}

	void set_conf_file_path(const std::string& conf_file_path);

	void set_pcap_file_path(const std::string& pcap_file_path);

	void set_dst_addr(const std::string& dst_addr);

	void set_dst_port(const std::string& dst_port);

	void set_concurrency_limit(const std::string& concurrency_limit);

	void set_onoff_random_port(const std::string& onoff_random_port);

private:
	void read_in(const std::string& conf_file_path);

private:
	std::string  _conf_file_path;   ///< self-explanatory.

	std::string  _pcap_file_path;   ///< load traffic from the pcap file.

	std::string  _dst_addr;         ///< the desination IP address to which traffic will be forward.

	std::string  _dst_port;         ///< the destination port to which traffic will be forward.
	unsigned short _dst_port_num;

	std::string  _concurrency_limit;  ///< the maximum number of concurrent TCP connections.
	int _concurrency_limit_num;

	std::string  _onoff_random_port;    ///< specify if the source port of TCP segment will be replaced with a random number.
	bool _onoff_random_port_boolean;
};

#endif /* _CONFIGURATION_H_ */
