#include "net.h"
#include <fcntl.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

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
