#ifndef __NET_H__
#define __NET_H__ 
#include <string>
#include <netinet/in.h>

class net 
{
	public:
		static int setNonBlock(int fd);
		static int setReuseAddr(int fd);
		static int setReusePort(int fd);
		static int setNoDelay(int fd);
};

class Ip4Addr
{
	public:
		Ip4Addr(const std::string& host, short port);
		Ip4Addr(short port = 0):Ip4Addr("",port){}
		std::string toString() const;
		std::string ip() const;
		short port() const;
		unsigned int ipInt() const;
		bool isIpValid() const;
		struct sockaddr_in& getAddr(){return _addr;}
		static std::string hostToIp(const std::string& host){Ip4Addr addr(host,0);return addr.ip();}
	private:
		struct sockaddr_in _addr;
};


#endif
