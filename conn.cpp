#include "conn.h"

TcpConn::TcpConn()
	:_base(NULL),_channel(NULL),_state(State::Invalid),_dstPort(-1),
	_connectTimeout(0),_reconnectInterval(-1),_connectedTime(util::timeMilli())
{

}
TcpConn::~TcpConn()
{
	printf("tcp connection destroyed %s-->>%s\n",_local.toString().c_str(),_peer.toString().c_str());
	delete _channel;
}
TcpServer::TcpServer(EventBases* bases)
	:_bases(bases),
	_listen_channel(NULL),
	_createcb([]{return TcpConnPtr(new TcpConn);})
{}
