#ifndef __UTIL_H__
#define __UTIL_H__

#include <iostream>
#include <set>

class noncopyable
{
	public:
		noncopyable(){}
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator= (const noncopyable&) = delete;
};

typedef std::pair<int64_t, int64_t> TimerId;

struct util 
{
	static int64_t timeMicro();
	static int64_t timeMilli(){return timeMicro() / 1000;}
	static int64_t steadyMicro();
	static int64_t steadyMilli(){return steadyMicro() / 1000;}
	static int addFdFlag(int fd, int flag);
};
#endif
