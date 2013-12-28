/*********************************************
 * sessionmanager.h
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 9, 2013
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
	
	int get_ready();

	void erase_a_session(uint64_t key);

	void inject_a_realtime_ippkt(const char* ip_pkt);

private:
	/**
	 * weed out tcpsession with unhealthy IP samples.
	 */
	int clean();
		
	/**
	 * IP packet will be dispatched to corresponding TCP session.
	 */
	int dispatch_ip_pkt(const char* ip_pkt);

private:
	std::map<uint64_t, tcpsession>  _sessions;
};

#endif /* _SESSIONMANAGER_H_ */
