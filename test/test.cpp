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
	int seconds;
	struct timespec req, rem;

	ret = horos_init("/home/kamus/projs/horoscope/public/traffic.smallsize.pcap",
			"192.168.44.129", 80, 10);
	if (ret != 0)
	{
		cerr << "horos_init() failed.\n";
		return ret;
	}
	cout << "horos is working...\n";

	// sleep for a moment...
	req.tv_sec = 20;  // in unit of second.
	req.tv_nsec = 0;
	do
	{
		nanosleep(&req, &rem);
		req = rem;
	}while(rem.tv_sec);

	horos_uninit();
	cout << "Quit...\n";

	return 0;
}




