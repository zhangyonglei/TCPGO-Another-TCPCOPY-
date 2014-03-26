/*********************************************
 * tcppostman.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Mar, 2014
 ********************************************/

#include "tcp_postman.h"

tcp_postman::tcp_postman(postoffice* po)
	: postman(po)
{
}

tcp_postman::~tcp_postman()
{
}

void tcp_postman::get_ready4subclass()
{
}

void tcp_postman::recv_impl()
{
}
