/*********************************************
 * main.c
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
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
 */

#include <getopt.h>
#include "misc.h"
#include "cute_logger.h"
#include "session_manager.h"
#include "postoffice.h"
#include "thetimer.h"
#include "poller.h"
#include "realtime_captureer.h"

using namespace std;

string   g_pcap_file_path;
string   g_dst_addr;
uint16_t g_dst_port;
int g_concurrency_limit = 20;  // default to 20 connections run concurrently.

static void output_help();
static void output_version();
int run();

int main(int argc, char **argv)
{
	int ch;
	int option_index;
	struct option long_options[] = {
			{"pcapfile", required_argument, NULL,  'f' },
			{"dst-addr", required_argument, NULL, 'd' },
			{"dst-port", required_argument, NULL, 'p'},
			{"concurrency-limit", required_argument, NULL, 'c'},
			{"help", no_argument, NULL, 'h'},
			{"version", no_argument, NULL,  'v' },
			{0, 0, 0, 0}
	};

	while (true) {
		ch = getopt_long(argc, argv, "f:d:p:c:hv", long_options, &option_index);
		if (ch == -1)
			break;

		switch (ch) {
		case 'f':
			g_pcap_file_path = optarg;
			break;

		case 'd':
			g_dst_addr = optarg;
			break;

		case 'p':
			g_dst_port = strtol(optarg, NULL, 10);
			break;

		case 'c':
			g_concurrency_limit = strtol(optarg, NULL, 10);
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

	if (argc == 1)
	{
		output_help();
	}
	else
	{
		run();
	}

	return 0;
}

static void output_version()
{
	cout << "Horoscope v0.1\n";
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

	srand(time(NULL));
	if (!g_pcap_file_path.empty())
	{
		ret = g_session_manager.read_from_capfile(g_pcap_file_path, "tcp");
		if (0 != ret)
		{
			g_logger.printf("failed to open pcap file %s.\n", g_pcap_file_path.c_str());
			return ret;
		}
	}

	g_session_manager.get_ready();
	g_postoffice.get_ready();
	g_realtime_captureer.get_ready();
	g_timer.get_ready();
	/// the word starts from a big bang.Refer to function declarations for yummy explanations.
	g_poller.bigbang();
}
