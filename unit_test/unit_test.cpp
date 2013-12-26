/*********************************************
 * unit_test.cpp
 * Author: kamuszhou@tencent.com 16236914@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Dec, 2013
 * I'm not a christian. I don't celebrate the christmas day.
 ********************************************/

#include "../public/horos.h"
#include "../public/misc.h"
#include <iostream>

using namespace std;

int main(int argc, char** argv)
{
	int ret;

	ret = horos_init("/home/kamus/projs/horoscope/public/traffic.smallsize.pcap",
			"192.168.44.129", 80, 10);
	if (ret != 0)
	{
		cerr << "horos_init() failed.";
		return ret;
	}

	return 0;
}




