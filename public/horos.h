/*********************************************
 * horos.h
 * Author: kamuszhou@tencent.com kamuszhou@qq.com
 * website: v.qq.com  www.dogeye.net
 * Created on: 25 Dec, 2013
 * I'm not a christian. I don't celebrate the christmas day.
 ********************************************/

#ifndef _HOROS_H_
#define _HOROS_H_

/**
 * This function returns immediately.
 * @param pcap_file_path The pcap file that will be loaded.
 * @param dst_ip_addr The destination IP address that traffic will be sent to.
 * @param dst_port The destination IP port that traffic will be sent to. in host byte order.
 * @param concurrency At least how many TCP connections will be established at the same time.
 * @return 0 on success, non-zero on failure.
 */
int horos_init();

/**
 * This function blocks until all resources has been cleaned.
 */
void horos_uninit();

#endif /* _HOROS_H_ */
