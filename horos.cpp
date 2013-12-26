/*********************************************
 * horos.cpp
 * Author: kamuszhou@tencent.com 16236914@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Dec, 2013
 * I'm not a christian. I don't celebrate the christmas day.
 ********************************************/

#include "horos.h"
#include "misc.h"

extern std::string  g_pcap_file_path;
extern std::string  g_dst_addr;
extern uint16_t g_dst_port;
extern int g_concurrency_limit;
extern int run();

__attribute__((visibility("default"))) int horos_init(const char* pcap_file_path,
		const char* dst_ip_addr, unsigned short dst_port, int concurrency)
{
	if (NULL == pcap_file_path || NULL == dst_ip_addr)
		return -1;

	g_pcap_file_path = pcap_file_path;
	g_dst_addr = dst_ip_addr;
	g_dst_port = dst_port;
	if (concurrency > 0)
		g_concurrency_limit = 20;
	else
		g_concurrency_limit = concurrency;

	return run();
}

__attribute__((visibility("default"))) void horos_uninit()
{
}
