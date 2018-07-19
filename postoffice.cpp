/*********************************************
 * postoffice.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: http://blog.ykyi.net
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
#include "statistics_bureau.h"

boost::mutex postoffice::_mutex;
std::vector<boost::shared_ptr<postoffice> > postoffice::_postoffices;

postoffice::postoffice(int asio_idx)
{
	_l2hdr_len = -1;
	_svr_port = htons(g_configuration.get_dst_port());
	_asio_idx = asio_idx;
}

postoffice::~postoffice()
{
}

postoffice& postoffice::instance(int idx)
{
	if (0 == _postoffices.size())
	{
		boost::lock_guard<boost::mutex> lock(_mutex);
		if (0 == _postoffices.size())
		{
			_postoffices.resize(g_configuration.get_asio_thrd_num());
			for (int i = 0; i < _postoffices.size(); i++)
			{
				_postoffices[i].reset(new postoffice(i));
			}
		}
	}

	return *_postoffices[idx].get();
}

void postoffice::get_ready(boost::shared_ptr<postman> pm)
{
	_asio_thrd_num = g_configuration.get_asio_thrd_num();
	employ_a_postman(pm);
}

void postoffice::employ_a_postman(boost::shared_ptr<postman> pm)
{
	_postman = pm;
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
		gotya = _postman->recv(_asio_idx, pkt);
		if (gotya)
		{
			// now inform the corresponding receiver the coming ip package.
			key = pkt->get_sess_key_inbound();
			ite = _callbacks.find(key);
			if (ite != _callbacks.end())
			{
				(*ite)->got_a_packet(pkt);
			}
		}
		else
		{
			return;
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
	if (concurrency_limit_num != 1)
	{
		concurrency_limit_num = (concurrency_limit_num + _asio_thrd_num)/_asio_thrd_num;
	}
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
			session_manager::instance(_asio_idx).erase_a_session(_callbacks.get_key(ite));
			_callbacks.erase(ite++);
			continue;
		}
/*		else if (postoffice_callback_interface::IGNORE == num)
		{
			++ite;       obsoleted !
			continue;
		}*/
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
				success = _postman->send(_asio_idx, pkt);
				// std::cout << pkt->get_sent_counter() << "-----\n";
				// success = _postman->send_sync(pkt);
			}
			else
			{

			}

			if (success)
			{
				g_statistics_bureau.inc_total_transmit_count();
				if (0 == pkt->get_send_counter())   // the first time to send this packet.
				{
					g_statistics_bureau.inc_unique_transmit_count();
				}

				pkt->mark_me_has_been_sent();
				pkt->increment_send_counter();
				pkt->set_last_recorded_snd_time();
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

