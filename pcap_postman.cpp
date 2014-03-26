/*********************************************
 * pcap_postman.cpp
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Mar, 2014
 ********************************************/

#include "pcap_postman.h"

pcap_postman::pcap_postman(postoffice* po)
	: postman(po)
{
}

pcap_postman::~pcap_postman()
{
}

void pcap_postman::get_ready4subclass()
{
}

void pcap_postman::recv_impl()
{
}
