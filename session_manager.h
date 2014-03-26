/*********************************************
 * sessionmanager.h
 * Author: kamuszhou@tencent.com, kamuszhou@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
 * Praise Be to the Lord. BUG-FREE CODE !
 ********************************************/

#ifndef _SESSIONMANAGER_H_
#define _SESSIONMANAGER_H_

#include "misc.h"
#include "tcpsession.h"

class session_manager;
extern session_manager g_session_manager;

class session_manager {
public:
	session_manager();
	virtual ~session_manager();

	int read_from_capfile(const std::string& path, const std::string& filter);
	void inject_a_realtime_ippkt(boost::shared_ptr<ip_pkt> pkt);
	
	int get_ready();

	void erase_a_session(uint64_t key);

	int size()
	{
		return _sessions.size();
	}

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
	std::map<uint64_t, tcpsession>  _sessions;

	static const int SESSION_COUNT_CEILING = 30928;  // magic number.
	bool _traffic_jam;
};

#endif /* _SESSIONMANAGER_H_ */
