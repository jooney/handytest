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
		Ip4Addr(const struct sockaddr_in& addr):_addr(addr){}
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

class Buffer
{
	public:
		Buffer():_buf(NULL),_b(0),_e(0),_cap(0),_exp(512){}
		~Buffer() {delete []_buf;}
		void clear() {delete[] _buf; _buf = NULL;_b = _e = _cap = 0;}
		size_t size() const{return _e - _b;}
		bool empty() const {return _b == _e;}
		char* data() const {return _buf+_b;}
		char* begin() const {return _buf + _b;}
		char* end() const {return _buf+_e;}
		char* makeRoom(size_t len);
		void  makeRoom();
		size_t space() const {return _cap - _e;}
		void addSize(size_t len) {_e += len;}
		char* allocRoom(size_t len){char* p = makeRoom(len);addSize(len);return p;}
		Buffer& append(const char* p, size_t len);
		Buffer& append(const char* p);
		void setSuggestSize(size_t sz) {_exp = sz;}
		Buffer& consume(size_t len);
		Buffer& absorb(Buffer& buf);
	private:
		char* _buf;
		size_t _b, _e ,_cap, _exp;
	private:
		void moveHead();
		void expand(size_t len);
		void copyFrom(const Buffer&);
};


#endif
