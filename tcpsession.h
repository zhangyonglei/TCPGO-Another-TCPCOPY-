/*********************************************
 * tcpsession.h
 * Author: kamuszhou www.dogeye.net
 * Created on: Dec 9, 2013
 ********************************************/

#ifndef _TCPSESSION_H_
#define _TCPSESSION_H_

#include <list>

class mem_t;

class tcpsession {
public:
	tcpsession();
	virtual ~tcpsession();

	void append_ip_sample(const unsigned char* ippkg);

private:
	std::list<mem_t>  _ippkgs_samples;    ///< The ip packages which will be used to emulate the pseudo-client.
	std::list<mem_t>  _ippkgs_received;   ///< The ip packages received from the server will be saved here.
};

#endif /* _TCPSESSION_H_ */
