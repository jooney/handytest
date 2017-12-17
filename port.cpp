#include "port.h"
#include <netdb.h>
#include <string.h>
#include <sys/syscall.h>
#include <pthread.h>

struct in_addr getHostByName(const std::string& host)
{
	struct in_addr addr;
	char buf[1024];
	struct hostent hent;
	struct hostent* he = NULL;
	int herrno = 0;
	memset(&hent,0,sizeof hent);
	int r = gethostbyname_r(host.c_str(),&hent, buf,sizeof buf, &he,&herrno);
	if (r == 0 && he->h_addrtype == AF_INET)
		addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
	else 
		addr.s_addr = INADDR_NONE;
	return addr;
}

uint64_t gettid()
{
	pthread_t tid = pthread_self();
	return tid;
}
