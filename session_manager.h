/*********************************************
 * sessionmanager.h
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: http://blog.ykyi.net
 * Created on: Dec 9, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _SESSIONMANAGER_H_
#define _SESSIONMANAGER_H_

#include <boost/unordered_map.hpp>
#include "misc.h"
#include "tcpsession.h"

class session_manager
{
private:
	session_manager(int asio_idx);

public:
	virtual ~session_manager();

public:
	static session_manager& instance(int idx);

	static int read_from_pcapfile(const std::string& path, const std::string& filter);
	void inject_a_realtime_ippkt(boost::shared_ptr<ip_pkt> pkt);
	
	int get_ready();

	void erase_a_session(uint64_t key);

	int get_healthy_sess_count()
	{
		return _healthy_sess_count;
	}

	void increase_healthy_sess_count()
	{
		_healthy_sess_count++;
	}

	void decrease_healthy_sess_count()
	{
		_healthy_sess_count--;
	}

	bool is_in_traffic_jam_control()
	{
		return _traffic_jam;
	}

	int size()
	{
		return _sessions.size();
	}

	// used for clone.
	void clone_sessions(tcpsession& sess);

private:
	/**
	 * weed out tcpsession with unhealthy IP samples.
	 */
	int clean_sick_session();
		
	/**
	 * IP packet will be dispatched to corresponding TCP session.
	 */
	void dispatch_ip_pkt(boost::shared_ptr<ip_pkt> pkt);

private:
	typedef std::map<uint64_t, boost::shared_ptr<tcpsession> > SessMap;
	SessMap  _sessions;

	//typedef boost::unordered_map<uint64_t, tcpsession> SessMap;
	//boost::unordered_map<uint64_t, tcpsession> _sessions;

	int  _expected_qps;
	int  _session_count_limit;
	int  _healthy_sess_count;  ///< session has all packets needed without lossing packets.
	bool _traffic_jam;

	int _asio_idx;

	static boost::mutex _mutex;
	static std::vector<boost::shared_ptr<session_manager> > _managers;
};

#endif /* _SESSIONMANAGER_H_ */
