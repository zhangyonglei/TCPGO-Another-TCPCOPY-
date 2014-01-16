/*********************************************
 * configuration.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 16 Jan, 2014
 ********************************************/

#include "misc.h"
#include "configuration.h"
#include "INIReaderException.h"
#include "INIWriterException.h"
#include "cute_logger.h"

configuration g_configuration;

using namespace cppiniparser;

configuration::configuration()
{
	set_conf_file_path("my.conf");
	set_concurrency_limit("100");
	set_onoff_random_port("1");
}

configuration::~configuration()
{
}

bool configuration::check_validity()
{
	if(!_dst_addr.empty())
	{
		g_logger.printf("dst_addr was not set.\n");
		abort();
		return false;
	}

	if(!_dst_port.empty())
	{
		g_logger.printf("dst_port was not set.\n");
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
	_dst_addr = dst_addr;
}

void configuration::set_dst_port(const std::string& dst_port)
{
	char buff[256];

	_dst_port_num = strtol(dst_port.c_str(), NULL, 10);
	assert(0 != _dst_port_num);
	snprintf(buff, sizeof(buff), "%d", _dst_port_num);
	_dst_port = buff;
}

void configuration::set_concurrency_limit(const std::string& concurrency_limit)
{
	char buff[256];

	_concurrency_limit_num = strtol(concurrency_limit.c_str(), NULL, 10);
	assert(0 != _concurrency_limit_num);
	snprintf(buff, sizeof(buff), "%d", _concurrency_limit_num);
	_concurrency_limit = buff;
}

void configuration::set_onoff_random_port(const std::string& onoff_random_port)
{
	if (onoff_random_port == "0" || onoff_random_port == "off" || onoff_random_port.empty())
	{
		_onoff_random_port_boolean = false;
		_onoff_random_port = "0";
	}
	else
	{
		_onoff_random_port_boolean = true;
		_onoff_random_port = "1";
	}
}

void configuration::read_in(const std::string& conf_file_path)
{
	INIConfig config;

	_conf_file_path = conf_file_path;

	try
	{
		config = INIParser::Read(conf_file_path.c_str());
	}
	catch(INIReaderException& e)
	{
		_conf_file_path.clear();
		g_logger.printf("%s\n", e.what());
		return;
	}

	std::string section_name;
	std::string option_name;
	std::string value;
	section_name = "MAIN";

	if (!config.HasSection(section_name))
		return;

	option_name = "pcap_file_path";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		set_pcap_file_path(value);
	}

	option_name = "dst_addr";
	if (config.HasOption(section_name, option_name))
	{
		value = config.GetOption(section_name, option_name);
		set_dst_addr(value);
	}

	option_name = "dst_port";
	if (config.HasOption(section_name, option_name))
	{
		value = config.HasOption(section_name, option_name);
		set_dst_addr(value);
	}

	option_name = "concurrency_limit";
	if (config.HasOption(section_name, option_name))
	{
		value = config.HasOption(section_name, option_name);
		set_concurrency_limit(value);
	}

	option_name = "onoff_random_port";
	if (config.HasOption(section_name, option_name))
	{
		value = config.HasOption(section_name, option_name);
		set_onoff_random_port(value);
	}
}
