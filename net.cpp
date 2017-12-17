#include "net.h"
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string.h>
#include "port.h"

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
