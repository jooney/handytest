#include "Event_base.h"
#include "conn.h"
#include "log.h"
int main()
{
	Logger::getLogger().setFileName("tcpserver.log");	
	Logger::getLogger().setLogLevel("DEBUG");
	EventBase base;
//	MultiBase  base(3);
	//printf("EventBase addr[%p]\n",&base);
	info("*******************************************************");
	info("*******************************************************");
	info("*******************************************************");
	info("**********************start Tcp Server*****************");
	TcpServerPtr svr = TcpServer::startServer(&base,"127.0.0.1",8899);
	base.loop();
	return 0;
}
