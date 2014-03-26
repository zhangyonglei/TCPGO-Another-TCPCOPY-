/*********************************************
 * postoffice.cpp
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
#include "rawsock_postman.h"
#include "pcap_postman.h"
#include "tcp_postman.h"

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

	switch (g_configuration.get_sniff_method())
	{
	case configuration::SNIFF_RAW:
		_postman.reset(new rawsock_postman(this));
		break;

	case configuration::SNIFF_PCAP:
		_postman.reset(new pcap_postman(this));
		break;

	case configuration::SNIFF_TCP:
		_postman.reset(new tcp_postman(this));
		break;
	}
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
	bool gotya;
	uint64_t key;
	uint16_t src_port;
	boost::shared_ptr<ip_pkt> pkt;
	postoffice_callback_interface* callback;
	mylistmap::iterator ite;

	while(true)
	{
		gotya = _postman->recv(pkt);
		if (!gotya)
			return;

		// now inform the corresponding receiver the coming ip package.
		key = pkt->get_sess_key_inbound();
		ite = _callbacks.find(key);
		if (ite != _callbacks.end())
		{
			(*ite)->got_a_packet(pkt);
		}
	}
}

void postoffice::send_packets_to_wire()
{
	bool success;
	int tot_len;
	mylistmap::iterator ite;
	postoffice_callback_interface* callback;
	int  concurrency_num;
	int  concurrency_limit_num;
	bool data_has_been_sent;
	boost::shared_ptr<ip_pkt> pkt;

	if (_callbacks.empty())
	{
		return;
	}

	concurrency_num = 0;
	data_has_been_sent = false;
	concurrency_limit_num = g_configuration.get_concurrency_limit();
	// practically, loop through all the tcpsessions.
	for(ite = _callbacks.begin(); ite != _callbacks.end(); )
	{
		if (0 != concurrency_limit_num && concurrency_num >= concurrency_limit_num)
			break;

		callback = *ite;

		int num;
		std::vector<boost::shared_ptr<ip_pkt> > pkts;
		num = callback->pls_send_these_packets(pkts);

		if (postoffice_callback_interface::REMOVE == num)
		{
			g_session_manager.erase_a_session(_callbacks.get_key(ite));
			_callbacks.erase(ite++);
			continue;
		}
		else if (postoffice_callback_interface::IGNORE == num)
		{
			++ite;
			continue;
		}
		else if (0 == num)
		{
			++ite;
			++concurrency_num;
			continue;
		}
		else if (num < 0)
		{
			// not expected to reach here.
			abort();
		}

		++concurrency_num;
		data_has_been_sent = true;
		for (int i = 0; i < num; i++)
		{
			pkt = pkts[i];

			success = false;
			if (pkt->should_send_me())
			{
				success = _postman->send(pkt);
			}

			if (success)
			{
				pkt->mark_me_has_been_sent();
				pkt->increment_sent_counter();
			}
			else
			{
				return;
			}
		}
		++ite;
	}  // end of for loop ...

	// punish the sending logic if no data has been sent in this around.
	// temporarily unregister the POLLOUT event.
//	if (!data_has_been_sent)
//	{
//		_postman->punish_sender(HZ / 10);
//	}
}

