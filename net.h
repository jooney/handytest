#ifndef __NET_H__
#define __NET_H__ 

class net 
{
	public:
		static int setNonBlock(int fd);
		static int setReuseAddr(int fd);
		static int setReusePort(int fd);
		static int setNoDelay(int fd);
};


#endif
