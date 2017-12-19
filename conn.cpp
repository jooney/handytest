#include "conn.h"
#include <fcntl.h>
#include <unistd.h>
#include "Epoller.h"
#include <poll.h>
#include <string.h>
#include "log.h"

TcpConn::TcpConn()
	:_base(NULL),_channel(NULL),_state(State::Invalid),_dstPort(-1),
	_connectTimeout(0),_reconnectInterval(-1),_connectedTime(util::timeMilli())
{
	memset(_writeBuffer,0,sizeof(_writeBuffer));
}
TcpConn::~TcpConn()
{
	debug("tcp connection destroyed [%s]-->>[%s]",_local.toString().c_str(),_peer.toString().c_str());
	delete _channel;
}
void TcpConn::attach(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer)
{
	_base = base;
	_state = State::Handshaking;
	_local = local;
	_peer = peer;
	_channel = new Channel(base, fd,kReadEvent);
	debug("tcp constructed [%s]----->>>>[%s] fd[%d]\n",_local.toString().c_str(),_peer.toString().c_str(),fd);
	TcpConnPtr conn = shared_from_this();
	conn->_channel->onRead([=]{conn->handleRead(conn);});//Channel::onRead -> TcpConn::handleRead
	conn->_channel->onWrite([=]{conn->handleWrite(conn);});//Channel::onWrite->TcpConn::handleWrite
}

int TcpConn::handleHandshake(const TcpConnPtr& conn)
{
	assert(_state != State::Handshaking);
	struct pollfd pfd;
	pfd.fd = _channel->fd();
	pfd.events = POLLOUT | POLLERR;
	int r = poll(&pfd,1,0);
	if (r== 1 && pfd.revents == POLLOUT)
	{
	//	_channel->enableReadWrite(true,false);
		_state = State::Connected;
		if (_statecb)
		{
			_statecb(conn);
		}
	}
	else{
		debug("poll fd[%d] return [%d] revents[%d]\n",_channel->fd(),r,pfd.revents);
		//cleanup()  TODO
		return -1;
	}
	return 0;
}
void TcpConn::handleRead(const TcpConnPtr& conn)
{
//	if (_state == State::Handshaking && handleHandshake(conn))
//	{
//		printf("handleRead quit\n");
//		return;
//	}
//	while(_state == State::Connected)
	if (1)
	{
		int ret;
		char buf[1024];
		memset(buf,0,sizeof buf);
		if (_channel->fd() >= 0)
		{
			ret = ::read(_channel->fd(),buf,sizeof buf);
			debug("channel id[%lld] fd[%d] readed [%d]bytes contents[%s]\n",
					_channel->id(),_channel->fd(),ret,buf);
			memcpy(_writeBuffer,buf,strlen(buf));
			_channel->enableWrite(true);
		}
	}
}
void TcpConn::handleWrite(const TcpConnPtr& conn)
{
	if (_channel->writeEnabled())
	{
		Send(_writeBuffer,strlen(_writeBuffer));	
		_channel->enableWrite(false); //disable _poller write
		memset(_writeBuffer,0,sizeof _writeBuffer);
	}
	//if (_state == State::Handshaking)
	//{
	//	printf("handleWrite quit\n");
	//	return;
	//}
	//else if (_state == State::Connected)
	//{
	//	if (_channel->writeEnabled())
	//	{
	//		_channel->enableWrite(false); //disable _poller write 
	//	}
	//}
	//else{
	//	printf("handle write unexpedted\n");
	//}
}
ssize_t TcpConn::iSend(const char* buf, size_t len)
{
	size_t sended = 0;
	while (len > sended)
	{
		ssize_t wd = ::write(_channel->fd(),buf + sended, len - sended);
		debug("channel id[%lld] fd[%d] write[%d] bytes\n",(long long)_channel->id(),
				_channel->fd(),wd);
		if (wd > 0)
		{
			sended += wd;
			continue;
		}
		else if (wd == -1 && errno == EINTR){
			continue;
		}
		else if(wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			if (!_channel->writeEnabled()){
				_channel->enableWrite(true);//notify _poller
			}
			break;
		}
		else
		{
			debug("write error: channel id[%lld] fd[%d] wd[%d]\n",(long long)_channel->id(),_channel->fd(),wd);
		}
	}
	return sended;
}
void TcpConn::Send(const char* str,size_t len)
{
	if (_channel)
	{
		if (_channel->writeEnabled())
		{
			ssize_t nSend = iSend(str,len);
			debug("TcpConn::Send succ[%d] bytes\n",nSend);
		}
	}
	else{
		debug("TcpConn::Send channel doesn't exist\n");
	}
}
TcpServer::TcpServer(EventBases* bases)
	:_bases(bases),
	_base(bases->allocBase()),
	_listen_channel(NULL),
	_createcb([]{return TcpConnPtr(new TcpConn);})
{}

int TcpServer::bind(const std::string& host,short port)
{
	_addr = Ip4Addr(host,port);
	int fd = ::socket(AF_INET, SOCK_STREAM,0);
	int r = net::setReuseAddr(fd);
	r = util::addFdFlag(fd,FD_CLOEXEC);
	r = ::bind(fd,(struct sockaddr*)&_addr.getAddr(),sizeof(struct sockaddr));
	if (r)
	{
		::close(fd);
		error("failed to bind %s\n",_addr.toString().c_str());
		return -1;
	}
	r = ::listen(fd,20);
	debug("fd[%d] listening at %s\n",fd,_addr.toString().c_str());
	_listen_channel = new Channel(_base, fd,kReadEvent);
	_listen_channel->onRead([this]{handleAccept();});
	return 0;
}

TcpServerPtr TcpServer::startServer(EventBases* bases, const std::string& host, short port, bool reusePort)
{
	TcpServerPtr p(new TcpServer(bases));
	int r = p->bind(host,port);
	return r ==0 ? p:NULL;
}

void TcpServer::handleAccept()
{
	struct sockaddr_in raddr;
	socklen_t rsz = sizeof (raddr);
	int lfd = _listen_channel->fd();
	int cfd;
	while (lfd >= 0 &&(cfd = ::accept(lfd,(struct sockaddr*)&raddr,&rsz))>=0)
	{
		sockaddr_in peer, local;
		socklen_t alen = sizeof(peer);
		int r = getpeername(cfd,(sockaddr*)&peer,&alen);
		if (r <0){
			error("get peer name failed\n");
			continue;
		}
		r = getsockname(cfd,(sockaddr*)&local,&alen);
		if (r <0){
			error("getsockname failed\n");
			continue;
		}
		r = util::addFdFlag(cfd,FD_CLOEXEC);
		EventBase* b = _bases->allocBase();
		debug("TcpServer::handleAccept EventBase addr[%p]\n",b);
		auto addcon = [=]{
			TcpConnPtr conn = _createcb();
			conn->attach(b,cfd,local,peer);
		//	if (_statecb)
		//	{
		//		conn->onState(_statecb);	
		//	}
		//	if (_readcb)
		//	{
		//		conn->onRead(_readcb);
		//	}
		};
		if (b == _base)
		{
			addcon();
		}
		else{
			b->safeCall(std::move(addcon));
		}
	}
}
