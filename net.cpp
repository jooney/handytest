#include "net.h"
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string.h>
#include "port.h"
#include "Util.h"

int net::setNonBlock(int fd)
{
	int flags = ::fcntl(fd, F_GETFL,0);
	if (flags < 0)
		return errno;
	return ::fcntl(fd,F_SETFL,flags | O_NONBLOCK);
}

int net::setReuseAddr(int fd)
{
	int flag = true;
	return ::setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&flag, sizeof(flag));
}

int net::setReusePort(int fd)
{
	int flag = true;
	return ::setsockopt(fd,SOL_SOCKET,SO_REUSEPORT,&flag,sizeof(flag));
}

int net::setNoDelay(int fd)
{
	int flag = true;
	return setsockopt(fd,SOL_SOCKET,TCP_NODELAY,&flag, sizeof(flag));
}

Ip4Addr::Ip4Addr(const std::string& host, short port)
{
	memset(&_addr, 0, sizeof _addr);
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port);
	if (host.size()){
		_addr.sin_addr = getHostByName(host); 
	}else{
		_addr.sin_addr.s_addr = INADDR_ANY;
	}
	if (_addr.sin_addr.s_addr == INADDR_NONE){
		printf("cannot resove %s to ip", host.c_str());
	}
}

std::string Ip4Addr::toString() const
{
	uint32_t uip = _addr.sin_addr.s_addr;
	return util::format("%d.%d.%d.%d.%d",
		(uip>>0)&0xFF,
		(uip>>8)&0xFF,
		(uip>>16)&0xFF,
		(uip>>24)&0xFF,
		ntohs(_addr.sin_port));
}


char* Buffer::makeRoom(size_t len)
{
	if (_e + len < _cap)
	{
	}
	else if(size() + len < _cap / 2)
	{
		moveHead();
	}
	else{
		expand(len);
	}
	return end();
}
void Buffer::makeRoom()
{
	if (space() < _exp)
		expand(0);
}

void Buffer::moveHead()
{
	std::copy(begin(),end(),_buf);
	_e -= _b;
	_b = 0;
}

void Buffer::expand(size_t len)
{
	size_t ncap = std::max(_exp,std::max(size()+len,2*_cap));
	char* p = new char[ncap];
	std::copy(begin(),end(),p);
	_e -= _b;
	_b = 0;
	delete[] _buf;
	_buf = p;
	_cap = ncap;
}

Buffer& Buffer::append(const char* p, size_t len)
{
	memcpy(makeRoom(len),p,len);
	return *this;
}

Buffer& Buffer::append(const char* p)
{
	memcpy(makeRoom(strlen(p)),p,strlen(p));
	return *this;
}

Buffer& Buffer::consume(size_t len)
{
	_b += len;
	if (size() == 0)
		clear();
	return *this;
}

void Buffer::copyFrom(const Buffer& ref)
{
	memcpy(this,&ref, sizeof ref);
	if (ref._buf)
	{
		_buf = new char[ref._cap];
		memcpy(_buf,ref.begin(),ref.size());//?????
	}
}
Buffer& Buffer::absorb(Buffer& buf)
{
	if (&buf != this)
	{
		if (size() == 0)
		{
			char b[sizeof buf];
			memcpy(b,this,sizeof b);
			memcpy(this,&buf,sizeof b);
			memcpy(&buf,b,sizeof b);
			std::swap(_exp,buf._exp);
		}
		else{
			append(buf.begin(),buf.size());
			buf.clear();
		}
	}
	return *this;
}
