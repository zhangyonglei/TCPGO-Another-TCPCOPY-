/*********************************************
 * configuration.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 16 Jan, 2014
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include <boost/algorithm/string.hpp>
#include "misc.h"
#include "configuration.h"
#include "INIReaderException.h"
#include "INIWriterException.h"
#include "cute_logger.h"
#include "thetimer.h"

configuration g_configuration;

using namespace cppiniparser;

configuration::configuration()
{
	// the default settings.
	// [MAIN]
	set_conf_file_path("my.conf");
	set_dst_port(0);
	set_concurrency_limit(1000);
	set_onoff_random_port(true);
	set_accidental_death_pcap_file_limit(100);
	set_sniff_method(SNIFF_RAW);
	set_asio_thrd_num(-1);   // let the function deduce the best number by itself.
	set_pkt_pass_rate(1000);  // let all the packages pass.

	// [SESSION]
	set_session_count_limit(10000);
	set_response_from_peer_time_out(3 * HZ);
	set_have_to_send_data_within_this_timeperiod(3 * HZ);
	set_injecting_rt_traffic_timeout(4000);
	set_retransmit_time_interval(HZ / 4);
	set_wait_for_fin_from_peer_time_out(4 * HZ);
	set_enable_active_close(false);
	set_expected_qps(1000);
	set_clone(0);    // disable clone

	// [TESTSUITE]
	// if the test suite related path is not specified in configure file,
	// don't set it !
	// set_lua_scripts_home("/tmp");
	// set_so_home("/tmp");

	// [LOG]
	set_log_on(true);
	set_duplicate_log_to_stdout(true);
}

configuration::~configuration()
{
}

bool configuration::check_validity()
{
	if (_dst_addr.empty())
	{
		g_logger.printf("dst_addr was not set. Specify -d option on command line or set MAIN.dst_addr in conf file.\n");
		abort();
		return false;
	}

	if (0 == _dst_port)
	{
		g_logger.printf(
				"dst_port was not set. Specify -p option on command line or set MAIN.dst_port in conf file.\n");
		abort();
		return false;
	}

	return true;
}

void configuration::set_conf_file_path(const std::string& conf_file_path)
{
	_conf_file_path = conf_file_path;
}

void configuration::set_pcap_file_path(const std::string& pcap_file_path)
{
	_pcap_file_path = pcap_file_path;
}

void configuration::set_dst_addr(const std::string& dst_addr)
{
	int ret;
	struct in_addr inaddr;
	memset(&inaddr, 0, sizeof(inaddr));
	ret = inet_aton(dst_addr.c_str(), &inaddr);
	if (ret < 0)
	{
		perror(dst_addr.c_str());
		abort();
	}

	_dst_addr = dst_addr;
	_dst_addr_uint32 = *(uint32_t*)&inaddr;
}

void configuration::set_dst_port(const std::string& dst_port)
{
	_dst_port = strtol(dst_port.c_str(), NULL, 10);
}

void configuration::set_dst_port(unsigned short dst_port)
{
	_dst_port = dst_port;
}

void configuration::set_concurrency_limit(const std::string& concurrency_limit)
{
	_concurrency_limit = strtol(concurrency_limit.c_str(), NULL, 10);
}

void configuration::set_concurrency_limit(int concurrency_limit)
{
	_concurrency_limit = concurrency_limit;
}

void configuration::set_onoff_random_port(const std::string& onoff_random_port)
{
	if (onoff_random_port == "0" || onoff_random_port == "off"
			|| onoff_random_port.empty())
	{
		_onoff_random_port = false;
	}
	else
	{
		_onoff_random_port = true;
	}
}

void configuration::set_session_count_limit(const std::string& limit)
{
	int val;
	val = boost::lexical_cast<int>(limit);
	set_session_count_limit(val);
}

void configuration::set_session_count_limit(int limit)
{
	if (limit < 1)
	{
		std::cerr << "SESSION.session_count_limit is too small. " << limit << std::endl;
		return;
	}
	_session_count_limit = limit;
}

void configuration::set_onoff_random_port(bool onoff_random_port)
{
	_onoff_random_port = onoff_random_port;
}

void configuration::set_response_from_peer_time_out(const std::string& response_from_peer_time_out)
{
	int val;
	val = strtol(response_from_peer_time_out.c_str(), NULL, 10);
	set_response_from_peer_time_out(val);
}

void configuration::set_response_from_peer_time_out(int response_from_peer_time_out)
{
	assert(response_from_peer_time_out != 0);
	_response_from_peer_time_out = response_from_peer_time_out;
}

void configuration::set_have_to_send_data_within_this_timeperiod(const std::string& timeperiod)
{
	int val;
	val = strtol(timeperiod.c_str(), NULL, 10);
	set_have_to_send_data_within_this_timeperiod(val);
}

void configuration::set_have_to_send_data_within_this_timeperiod(int timeperiod)
{
	assert(timeperiod != 0);
	_have_to_send_data_within_this_timeperiod = timeperiod;
}

void configuration::set_injecting_rt_traffic_timeout(const std::string& timeout)
{
	int val;
	val = boost::lexical_cast<int>(timeout);
	set_injecting_rt_traffic_timeout(val);
}

void configuration::set_injecting_rt_traffic_timeout(int timeout)
{
	assert(timeout != 0);
	_injecting_rt_traffic_timeout = timeout;
}

void configuration::set_retransmit_time_interval(const std::string& speed_control)
{
	int val;
	val = strtol(speed_control.c_str(), NULL, 10);
	set_retransmit_time_interval(val);
}

void configuration::set_retransmit_time_interval(int speed_control)
{
	assert(speed_control != 0);
	_retransmit_time_interval = speed_control;
}

void configuration::set_wait_for_fin_from_peer_time_out(const std::string& time_out)
{
	int val;
	val = strtol(time_out.c_str(), NULL, 10);
	set_wait_for_fin_from_peer_time_out(val);
}

void configuration::set_wait_for_fin_from_peer_time_out(int time_out)
{
	assert(time_out != 0);
	_wait_for_fin_from_peer_time_out = time_out;
}

void configuration::set_enable_active_close(const std::string& enable)
{
	set_enable_active_close(enable != "0");
}

void configuration::set_enable_active_close(bool enable)
{
	_enable_active_close = enable;
}

void configuration::set_expected_qps(const std::string& qps)
{
	int val;
	val = boost::lexical_cast<int>(qps);
	set_expected_qps(qps);
}

void configuration::set_expected_qps(int qps)
{
	assert(qps > 0);
	_expected_qps = qps;
}

void configuration::set_clone(const std::string& clone)
{
	int val = boost::lexical_cast<int>(clone.c_str());
	set_clone(val);
}

void configuration::set_clone(int clone)
{
	assert(clone >= 0);
	_clone = clone;
}

void configuration::set_request_pattern(const std::string& pattern)
{
	if (pattern.empty())
	{
		return;
	}

	_request_pattern = boost::regex(pattern);
	// _request_pattern = boost::regex("\r\n\r\n");
}

void configuration::set_log_on(const std::string& log_on)
{
	set_log_on(log_on != "0");
}

void configuration::set_log_on(bool log_on)
{
	_log_on = log_on;
}

void configuration::set_duplicate_log_to_stdout(const std::string& dup)
{
	set_duplicate_log_to_stdout(dup != "0");
}

void configuration::set_duplicate_log_to_stdout(bool dup)
{
	_duplicate_log_to_stdout = dup;
}

void configuration::set_lua_scripts_home(const std::string& home)
{
	_lua_scripts_home = home;
}

void configuration::set_so_home(const std::string& home)
{
	_so_home = home;
}

void configuration::set_accidental_death_pcap_file_limit(const std::string& limit)
{
	int val;
	val = strtol(limit.c_str(), NULL, 10);
	set_accidental_death_pcap_file_limit(val);
}

void configuration::set_accidental_death_pcap_file_limit(int limit)
{
	_accidental_death_pcap_file_limit = limit;
}

void configuration::set_sniff_method(const std::string& sniff_method)
{
	std::string method(sniff_method);
	boost::to_upper(method);

	if ("RAW" == method)
	{
		_sniff_method = SNIFF_RAW;
	}
	else if ("PCAP" == method)
	{
		_sniff_method = SNIFF_PCAP;
	}
	else if("TCP" == method)
	{
		_sniff_method = SNIFF_TCP;
	}
	else
	{
		_sniff_method = SNIFF_RAW;
		g_logger.printf("%s is not a illegal sniff_method.\n");
	}
}

void configuration::set_asio_thrd_num(const std::string& num)
{
	int val;
	val = boost::lexical_cast<int>(num);
	set_asio_thrd_num(val);
}

void configuration::set_asio_thrd_num(int num)
{
	if (num <= 0)
	{
		int core_num = boost::thread::hardware_concurrency();
		_asio_thrd_num = core_num - 1;
		if (_asio_thrd_num < 2)
		{
			_asio_thrd_num = 2;
		}
	}
	else
	{
		_asio_thrd_num = num;
	}
}

void configuration::set_pkt_pass_rate(const std::string& rate)
{
	int val;
	val = boost::lexical_cast<int>(rate.c_str());
	set_pkt_pass_rate(val);
}

void configuration::set_pkt_pass_rate(int rate)
{
	_pkt_pass_rate = rate;
	if (_pkt_pass_rate < 0)
	{
		_pkt_pass_rate = 0;
	}
	else if (_pkt_pass_rate > 1000)
	{
		_pkt_pass_rate = 1000;
	}
}

void configuration::set_sniff_method(SNIFF_METHOD sniff_method)
{
	_sniff_method = sniff_method;
}

void configuration::readin()
{
	char buff[PATH_MAX];
	INIConfig config;

	if (NULL != getcwd(buff, sizeof(buff)))
	{
		g_logger.printf("Current Working Directory: %s\n", buff);
	}

	try
	{
		config = INIParser::Read(_conf_file_path.c_str());
		g_logger.printf("Read conf file: %s\n", _conf_file_path.c_str());
	}
	catch (INIReaderException& e)
	{
		_conf_file_path.clear();
		g_logger.printf("%s\n", e.what());

		return;
	}

	std::string section_name;
	std::string option_name;
	std::string value;

	// the MAIN section.
	section_name = "MAIN";

	if (config.HasSection(section_name))
	{
		g_logger.printf("MAIN section exists.\n");
	}

	option_name = "pcap_file_path";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("pcap_file_path: %s\n", value.c_str());
		set_pcap_file_path(value);
	}

	option_name = "dst_addr";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("dst_addr: %s\n", value.c_str());
		set_dst_addr(value);
	}

	option_name = "dst_port";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("dst_port: %s\n", value.c_str());
		set_dst_port(value);
	}

	option_name = "concurrency_limit";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("concurrency_limit: %s\n", value.c_str());
		set_concurrency_limit(value);
	}

	option_name = "onoff_random_port";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("onoff_random_port: %s\n", value.c_str());
		set_onoff_random_port(value);
	}

	option_name = "accidental_death_pcap_file_limit";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("accidental_death_pcap_file_limit: %s\n", value.c_str());
		set_accidental_death_pcap_file_limit(value);
	}

	option_name = "sniff_method";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("sniff_method: %s\n", value.c_str());
		set_sniff_method(value);
	}

	option_name = "asio_thrd_num";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("asio_thrd_num: %s\n", value.c_str());
		set_asio_thrd_num(value);
	}

	option_name = "pkt_pass_rate";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("pkt_pass_rate: %s\n", value.c_str());
		set_pkt_pass_rate(value);
	}

	// the session section
	section_name = "SESSION";

	if (config.HasSection(section_name))
	{
		g_logger.printf("SESSION section exists.\n");
	}

	option_name = "session_count_limit";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("session_count_limit: %s\n", value.c_str());
		set_session_count_limit(value);
	}

	option_name = "response_from_peer_time_out";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("response_from_peer_time_out: %s\n", value.c_str());
		set_response_from_peer_time_out(value);
	}

	option_name = "have_to_send_data_within_this_timeperiod";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("have_to_send_data_within_this_timeperiod: %s\n", value.c_str());
		set_have_to_send_data_within_this_timeperiod(value);
	}

	option_name = "injecting_rt_traffic_timeout";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("injecting_rt_traffic_timeout: %s\n", value.c_str());
		set_injecting_rt_traffic_timeout(value);
	}

	option_name = "retransmit_time_interval";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("retransmit_time_interval: %s\n", value.c_str());
		set_retransmit_time_interval(value);
	}

	option_name = "wait_for_fin_from_peer_time_out";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("wait_for_fin_from_peer_time_out: %s\n", value.c_str());
		set_wait_for_fin_from_peer_time_out(value);
	}

	option_name = "enable_active_close";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("enable_active_close: %s\n", value.c_str());
		set_enable_active_close(value);
	}

	option_name = "clone";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("clone: %s\n", value.c_str());
		set_clone(value);
	}

	option_name = "request_pattern";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("request_pattern: %s\n", value.c_str());
		set_request_pattern(value);
	}

	// the TESTSUITE session
	section_name = "TESTSUITE";

	if (config.HasSection(section_name))
	{
		g_logger.printf("TESTSUITE section exists.\n");
	}

	option_name = "lua_scripts_home";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("lua_scripts_home: %s\n", value.c_str());
		set_lua_scripts_home(value);
	}

	option_name = "so_home";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("so_home: %s\n", value.c_str());
		set_so_home(value);
	}

	// the LOG session
	section_name = "LOG";

	if (config.HasSection(section_name))
	{
		g_logger.printf("LOG section exists.\n");
	}

	option_name = "log_on";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("log_on: %s\n", value.c_str());
		set_log_on(value);
	}

	option_name = "duplicate_log_to_stdout";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		g_logger.printf("duplicate_log_to_stdout: %s\n", value.c_str());
		set_duplicate_log_to_stdout(value);
	}

	g_logger.printf("Finished reading conf file.\n");
	g_logger.flush();
}
