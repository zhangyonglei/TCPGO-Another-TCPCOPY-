/*********************************************
 * utils.cpp
 * Author: kamuszhou@tencent.com, 16236914@qq.com
 * website: www.dogeye.net
 * Created on: Dec 10, 2013
 ********************************************/

#ifndef _UTILS_H_
#define _UTILS_H_

#include "misc.h"

/**
 * the following macro is inspired by the book "Linux Kernel Development 3rd Edition" Page 214.
 * It gets around the problem of number wraparound.
 */
#define seq_before(unknown, known) ((int)(unknown) - (int)(known) < 0)

/**
 * calculate the IPv4 header checksum.
 */
unsigned short calc_ip_checksum(const unsigned short buff[], int len_ip_header);

/**
 *@param pcap The pcap handler.
 *@param frame The package captured on level 2.
 *@return The start address of IP package.
 */
const unsigned char* strip_l2head(pcap_t *pcap, const unsigned char *frame);

/**
 * detect the level2 header length in a ungraceful way. 
 */
int detect_l2head_len(const unsigned char *frame);

/* ip_pack is the starting address of a IP package. */
#define ip_packet_parser(ip_pack)        \
        struct iphdr *iphdr;   \
		struct tcphdr *tcphdr;                                     \
		unsigned char *tcp_content;                               \
		int ip_tot_len, iphdr_len, ip_content_len, tcphdr_len, tcp_content_len;     \
		\
		iphdr = (struct iphdr*)(ip_pack);                                   \
		ip_tot_len = ntohs(iphdr->tot_len);                                 \
		iphdr_len = iphdr->ihl << 2;                                          \
		ip_content_len = ip_tot_len - iphdr_len;                \
		tcphdr = (struct tcphdr*)((unsigned char*)iphdr + iphdr_len);   \
		tcphdr_len = tcphdr->doff << 2;                                                            \
		tcp_content = (unsigned char*)tcphdr + tcphdr_len;                \
		tcp_content_len = ip_content_len - tcphdr_len;                       \
		tcp_content_len = tcp_content_len; tcphdr_len = tcphdr_len;     \
		ip_tot_len = ip_tot_len; iphdr_len = iphdr_len; tcp_content = tcp_content;/* suppress the warning: set but not used */

#define ip_packet_clone(ip_pack)  ({   \
		int  tot_len;                                        \
		char  *pack_clone;                     \
		struct iphdr *iphdr;                  \
		iphdr = (struct iphdr*)ip_pack;       \
		tot_len = ntohs(iphdr->tot_len);    \
		pack_clone = new char[tot_len];        \
		memcpy(pack_clone, ip_pack, tot_len);    \
		pack_clone;   })

#define mk_sess_key(ip, port) ({    \
		uint64_t   k;                            \
		uint32_t *ptr;                       \
		ptr = (uint32_t*)&k;             \
		*ptr++ = ip;                           \
		*ptr = port;                            \
		k;   })

/// a block of memory.
class mem_t
{
public:
	mem_t()
{
		_mem = NULL;
		_mem_len = 0;
		_capacity_len = 0;
}

	mem_t(unsigned int sz)
	{
		_mem = NULL;
		_mem_len = 0;
		_capacity_len = 0;
		reserve(sz);
	}

	mem_t(const unsigned char* m, int len)
	{
		cp(m, len);
	}

	mem_t(const mem_t& m)
	{
		_mem = NULL;
		_mem_len = 0;
		_capacity_len = 0;
		reserve(m.get_capacity());
		cp(m.address(), m.get_capacity());
		_capacity_len = m._capacity_len;
		_mem_len = m._mem_len;
	}

	const mem_t& operator = (const mem_t& m)
	{
		delete _mem;
		_mem = NULL;
		_mem_len = 0;
		_capacity_len = 0;
		reserve(m.get_capacity());
		cp(m.address(), m.get_capacity());
		_capacity_len = m._capacity_len;
		_mem_len = m._mem_len;
		return *this;
	}

	~mem_t()
	{
		delete []_mem;
	}

	void reserve(unsigned int capacity)
	{
		if ( capacity > _capacity_len )  // it's not allowd to decrease the capacity.
		{
			unsigned char* save = _mem;
			_mem = new unsigned char[capacity];
			memset(_mem, 0, capacity);
			memcpy(_mem, save, _mem_len);
			_capacity_len = capacity;

			delete save;
		}
	}

	void resize(unsigned int size)
	{
		reserve(size);
		_mem_len = size;
	}

	void cp(const unsigned char* src, int len = -1)
	{
		_mem_len = 0;
		append(src, len);
	}

	void append(const unsigned char* src, int len = -1)
	{
		if ( -1 >= len )
		{
			len = strlen((char*)src);
		}

		int len_at_least = len + _mem_len;

		if ( len_at_least > _capacity_len )
		{
			reserve(len_at_least);
		}
		memcpy(_mem + _mem_len , src, len);
		_mem_len = len_at_least;
	}

	//unsigned char& operator[](unsigned int index)
	//{
	//	if ( index < _mem_len )
	//	{
	//		return _mem[index];
	//	}
	//	else
	//	{
	//		throw "index out of bounds in mem_t::operator[]";
	//	}
	//}

	unsigned char* address()const
	{
		return _mem;
	}

	int get_size()const
	{
		return _mem_len;
	}

	int get_capacity()const
	{
		return _capacity_len;
	}

	void clear()
	{
		delete _mem;
		_mem = NULL;
		_mem_len = 0;
		_capacity_len = 0;
	}

private:
	unsigned char * _mem;   ///< the start address
	uint  _mem_len;         ///< how many has been occupied.
	uint  _capacity_len;    ///< The maximum capacity.
};

#endif
