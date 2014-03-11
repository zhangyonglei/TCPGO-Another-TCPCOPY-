/*********************************************
 * sender.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: 11 Dec, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#include "misc.h"
#include "utils.h"
#include "cute_logger.h"
#include "postoffice.h"
#include "session_manager.h"
#include "configuration.h"

#define SNAP_LEN 8192

using namespace std;

postoffice g_postoffice;

postoffice::postoffice()
{
	_l2hdr_len = -1;
}

postoffice::~postoffice()
{
}

void postoffice::get_ready()
{
	_svr_port = htons(g_configuration.get_dst_port());
	//_postman.reset(new rawsock_postman(this));
	_postman.reset(new pcap_postman(this));
	_postman->get_ready();
}

void postoffice::register_callback(uint64_t key, postoffice_callback_interface* callback)
{
	mylistmap::iterator ite;
	ite = _callbacks.find(key);
	assert(ite == _callbacks.end());
	_callbacks[key] = callback;
}

void postoffice::deregister_callback(uint64_t key)
{
	mylistmap::iterator ite;
	ite = _callbacks.find(key);
	assert(ite != _callbacks.end());
	_callbacks.erase(ite);
}

void postoffice::recv_packets_from_wire()
{
	int ret;
	uint64_t key;
	uint16_t src_port;
	char *ptr_ippkt;
	postoffice_callback_interface* callback;
	mylistmap::iterator ite;

	while(true)
	{
		ret = _postman->recv(_buff, sizeof(_buff));
		if (ret < 0 && errno != EINTR)
			return;

		if (-1 == _l2hdr_len)
		{
			_l2hdr_len = detect_l2head_len(_buff);
			if (-1 == _l2hdr_len)
			{
				cerr << "Failed to detect data link level header's length.\n";
			}
		}

		ptr_ippkt = _buff + _l2hdr_len;
		ip_packet_parser(ptr_ippkt);
		// ip_packet_parser(_buff);

		// ignore the un-expected packages.
		if (ip_tot_len > ret)
		{
			continue;
		}

		if (iphdr->protocol != IPPROTO_TCP)
		{
			continue;
		}

		src_port = ntohs(tcphdr->source);
//		g_logger.printf("Got a tcp packet with source port %hu.\n", src_port);
		if (_svr_port != tcphdr->source)
		{
			continue;
		}

		// now inform the corresponding receiver the coming ip package.
		key = mk_sess_key(iphdr->daddr, tcphdr->dest);
		ite = _callbacks.find(key);
		if (ite != _callbacks.end())
		{
			ip_pkt pkt(ptr_ippkt);
			(*ite)->got_a_packet(&pkt);
		}
	}
}

void postoffice::send_packets_to_wire()
{
	int ret;
	const ip_pkt* pkt;
	const char* starting_addr;
	int tot_len;
	mylistmap::iterator ite;
	struct sockaddr_in  dst_addr;
	postoffice_callback_interface* callback;
	int  concurrency_num;
	int  concurrency_limit_num;
	bool data_has_been_sent;

	if (_callbacks.empty())
	{
//		cout << "All Finished. kamuszhou@tencent.com\n";
//		//		cout << "Would you please consider donating some QQ coins to kamuszhou, if you like this tool.\n";
//		cout << "Your support is greatly appreciated and will undoubted encourage me to devote more efforts "
//				"to make this gadget better." << endl;
//		exit(0);
	}

	dst_addr.sin_family = AF_INET;
	concurrency_num = 0;
	data_has_been_sent = false;
	concurrency_limit_num = g_configuration.get_concurrency_limit();
	// practically, loop through all the tcpsessions.
	for(ite = _callbacks.begin(); ite != _callbacks.end(); )
	{
		if (0 != concurrency_limit_num && concurrency_num >= concurrency_limit_num)
			break;

		++concurrency_num;
		callback = *ite;

		int num;
		vector<const ip_pkt*> pkts;
		num = callback->pls_send_these_packets(pkts);
		if (0 == num)
		{
			++ite;
			continue;
		}
		else if (-1 == num)
		{
			g_session_manager.erase_a_session(_callbacks.get_key(ite));
			_callbacks.erase(ite++);
			continue;
		}

		data_has_been_sent = true;
		for (int i = 0; i < num; i++)
		{
			pkt = pkts[i];
			dst_addr.sin_addr.s_addr = pkt->get_iphdr()->daddr;
			starting_addr = pkt->get_starting_addr();
			tot_len = pkt->get_tot_len();
			ret = _postman->sendto(starting_addr, tot_len, (struct sockaddr *)&dst_addr, sizeof(dst_addr));

			if (ret < 0 && errno == EINTR)
			{
				perror("send ");
				return;
			}
		}
		++ite;
	}  // end of for loop ...

	// punish the sending logic if no data has been sent in this around.
	// temporarily unregister the POLLOUT event.
	if (!data_has_been_sent)
	{
		_postman->punish_sender(HZ / 5);
	}
}

