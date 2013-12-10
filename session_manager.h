/*********************************************
 * sessionmanager.h
 * Author: kamuszhou www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#ifndef _SESSIONMANAGER_H_
#define _SESSIONMANAGER_H_

#include "misc.h"
#include "tcpsession.h"

class session_manager {
public:
	session_manager();
	virtual ~session_manager();

	int read_from_capfile(const std::string& path, const std::string& filter);
	
	int dispatch_ip_pkg(const u_char* ip_pkg);

private:
	std::map<uint64_t, tcpsession>  _sessions;
};

#endif /* _SESSIONMANAGER_H_ */
