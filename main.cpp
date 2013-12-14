/*********************************************
 * main.c
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#include <getopt.h>
#include "misc.h"
#include "cute_logger.h"
#include "session_manager.h"
#include "postoffice.h"
#include "thetimer.h"
#include "poller.h"

using namespace std;

string  g_pcap_file_path;

static void output_help();
static void output_version();
static void run();

int main(int argc, char **argv)
{
	int ch;
	int option_index;
	struct option long_options[] = {
			{"pcapfile", required_argument, NULL,  'f' },
			{"help", no_argument, NULL, 'h'},
			{"version", no_argument, NULL,  'v' },
			{0, 0, 0, 0}
	};

	while (true) {
		ch = getopt_long(argc, argv, "f:hv", long_options, &option_index);
		if (ch == -1)
			break;

		switch (ch) {
		case 'f':
			g_pcap_file_path = optarg;
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
	cout << "Copyright by Tencent Dec 9 2013 to " << __DATE__ << "\n" << endl;
}

static void output_help()
{
	cout << "Usage: \n";
	cout << "horos -f pcap_file_path.\n";
}

static void run()
{
	g_session_manager.read_from_capfile(g_pcap_file_path, "tcp");
	g_timer.get_ready();
	g_session_manager.get_ready();
	g_postoffice.get_ready();
	g_poller.run();   // thw word starts from now on ...........
}
