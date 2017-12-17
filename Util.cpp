#include "Util.h"
#include <fcntl.h>
#include <stdarg.h>
#include <chrono>
#include <memory>

std::string util::format(const char* fmt, ...) {
	char buffer[500];
	std::unique_ptr<char[]> release1;
	char* base;
	for (int iter = 0; iter < 2; iter++) {
		int bufsize;
		if (iter == 0) {
			bufsize = sizeof(buffer);
			base = buffer;
		} else {
			bufsize = 30000;
			base = new char[bufsize];
			release1.reset(base);
		}
		char* p = base;
		char* limit = base + bufsize;
		if (p < limit) {
			va_list ap;
			va_start(ap, fmt);
			p += vsnprintf(p, limit - p, fmt, ap);
			va_end(ap);
		}
		// Truncate to available space if necessary
		if (p >= limit) {
			if (iter == 0) {
				continue;       // Try again with larger buffer
			} else {
				p = limit - 1;
				*p = '\0';
			}
		}
		break;
	}
	return base;
}
int util::addFdFlag(int fd, int flag)
{
	int ret = ::fcntl(fd, F_GETFD); 
	return ::fcntl(fd,F_SETFD,ret | flag);
}

int64_t util::timeMicro()
{
	std::chrono::time_point<std::chrono::system_clock> p = std::chrono::system_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count();
}

int64_t util::steadyMicro()
{
	std::chrono::time_point<std::chrono::steady_clock> p = std::chrono::steady_clock::now();
	return std::chrono::duration_cast<std::chrono::microseconds>(p.time_since_epoch()).count();
}
