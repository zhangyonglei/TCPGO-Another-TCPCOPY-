/*********************************************
 * tcppostman.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Mar, 2014
 ********************************************/

#ifndef _TCPPOSTMAN_H_
#define _TCPPOSTMAN_H_

#include "postman.h"

class tcp_postman : public postman
{
public:
	tcp_postman(postoffice* po);
	virtual ~tcp_postman();

private:
	virtual void get_ready4subclass();
	virtual void recv_impl();
};

#endif /* _TCPPOSTMAN_H_ */
