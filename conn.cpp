#include "conn.h"

TcpServer::TcpServer(EventBases* bases)
	:_bases(bases),
	_listen_channel(NULL)
{}
