#ifndef __PORT_H__
#define __PORT_H__ 
#include <netinet/in.h>
#include <string>

struct in_addr getHostByName(const std::string& host);
uint64_t gettid();


#endif
