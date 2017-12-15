#ifndef __UTIL_H__
#define __UTIL_H__

class noncopyable
{
	public:
		noncopyable(){}
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator= (const noncopyable&) = delete;
};


#endif
