/*********************************************
 * main.c
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: v.qq.com www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/
 /*
 *
 *             _=,_
 *         o_/6 /#\  woof ! woof!! bark !!!
 *         \__ |##/
 *          ='|--\
 *            /   #'-.
 *            \#|_   _'-. /
 *             |/ \_( # |" 
 *            C/ ,--___/
 *
 *      www.tencent.com & www.dogeye.net co-present
 *      Praise Be to the Lord. BUG-FREE CODE !
 */

#include <getopt.h>
#include "misc.h"
#include "cute_logger.h"
#include "thetimer.h"
#include "reactor.h"
#include "proactor.h"
#include "politburo.h"
#include "realtime_capturer.h"
#include "configuration.h"
#include "mylua.h"
#include "statistics_bureau.h"
#include "testsuite.h"
#include "cascade.h"
#include "listmap.h"
#include "version.h"

using namespace std;

std::string conf_file_path;
std::string pcap_file_path;
std::string dst_addr;
std::string dst_port;
std::string concurrency_limit;
std::string onoff_random_port;

static void output_help();
static void output_version();
static void hook_here_for_tmp_test_code();

int run();

int main(int argc, char **argv)
{
	int ch;
	int option_index;
	struct option long_options[] = {
			{"conf", required_argument, NULL, 'x'},
			{"pcapfile", required_argument, NULL,  'f' },
			{"dst-addr", required_argument, NULL, 'd' },
			{"dst-port", required_argument, NULL, 'p'},
			{"concurrency-limit", required_argument, NULL, 'c'},
			{"random-port", required_argument, NULL, 'r'},
			{"help", no_argument, NULL, 'h'},
			{"version", no_argument, NULL,  'v' },
			{0, 0, 0, 0}
	};

	hook_here_for_tmp_test_code();

	while (true) {
		ch = getopt_long(argc, argv, "x:f:d:p:c:r:hv", long_options, &option_index);
		if (ch == -1)
			break;

		switch (ch)
		{
		case 'x':
			conf_file_path = optarg;
			break;

		case 'f':
			pcap_file_path = optarg;
			break;

		case 'd':
			dst_addr = optarg;
			break;

		case 'p':
			dst_port = optarg;
			break;

		case 'c':
			concurrency_limit = optarg;
			break;

		case 'r':
			onoff_random_port = optarg;
			break;

		case 'h':
			output_help();
			break;

		case 'v':
			output_version();
			break;

		case '?':
			break;

		default:
			cout << "getopt_long returned character code " << ch << ".\n";
			break;
		}
	}

	if (optind < argc) {
		cout << "unrecognized ARGV-elements:  ";
		while (optind < argc)
			cout << argv[optind++] << " ";
		cout << endl;
	}

	run();

	return 0;
}

static void output_version()
{
	cout << "Horoscope "VERSION_NUM"\n";
	cout << "This tool is aimed to replay captured client's requests to server.\n";
	cout << "Author: kamuszhou www.dogeye.net.\n";
	cout << "Copyrights Reserved by Tencent Dec 9 2013 to " << __DATE__ << "\n" << endl;
}

static void output_help()
{
	cout << "Usage: \n";
	cout << "horos [-f pcap_file_path] -d destination_ip -p port [-c concurrency]\n";
}

int run()
{
	int ret;
	std::string pcap_file_path;

	srand(time(NULL));

	if(!conf_file_path.empty())
	{
		g_configuration.set_conf_file_path(conf_file_path);
	}
	g_configuration.readin();

	if(!pcap_file_path.empty())
	{
		g_configuration.set_pcap_file_path(pcap_file_path);
	}
	if(!dst_addr.empty())
	{
		g_configuration.set_dst_addr(dst_addr);
	}
	if(!dst_port.empty())
	{
		g_configuration.set_dst_port(dst_port);
	}
	if(!concurrency_limit.empty())
	{
		g_configuration.set_concurrency_limit(concurrency_limit);
	}
	if(!onoff_random_port.empty())
	{
		g_configuration.set_onoff_random_port(onoff_random_port);
	}

	bool valid = g_configuration.check_validity();
	assert(valid);

	pcap_file_path = g_configuration.get_pcap_file_path();
	if (!pcap_file_path.empty())
	{
		ret = g_politburo.read_from_pcapfile(pcap_file_path, "tcp");
		if (0 != ret)
		{
			g_logger.printf("failed to open pcap file %s.\n", pcap_file_path.c_str());
			return ret;
		}
	}

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigaddset(&set, SIGUSR2);
    ret = sigprocmask(SIG_BLOCK, &set, NULL);
    assert(ret == 0);

	g_statistics_bureau.get_ready();
	g_mylua.get_ready();
	g_testsuite.ready_go();
	g_cascade.ready_go();
	g_realtime_capturer.get_ready();
	g_timer.get_ready();
	g_politburo.get_ready();
	// the story actually start from here.
	g_proactor.get_ready();
	// for a history reason, reactor once played the most important schedule role.
	// it's role has actually been replaced by proactor.
	// the word starts from a big bang.
	g_reactor.bigbang();

	return 0;
}

void hook_here_for_tmp_test_code()
{
}
