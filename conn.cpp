#include "conn.h"
#include <fcntl.h>
#include <unistd.h>
#include "Epoller.h"
#include <poll.h>
#include <string.h>
#include "log.h"

TcpConn::TcpConn()
	:_base(NULL),
	_channel(NULL),
	_readcb(NULL),
	_writecb(NULL),
	_state(State::Invalid),
	_dstPort(-1),
	_connectTimeout(0),
	_reconnectInterval(-1),
	_connectedTime(util::timeMilli())
{
}
TcpConn::~TcpConn()
{
	info("TcpConn::~TcpConn() tcp connection destroyed [%s]-->>[%s]",_local.toString().c_str(),_peer.toString().c_str());
//	if (_channel != NULL)
//		delete _channel;
}
void TcpConn::attach(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer)
{
	info("TcpConn::attach() fd[%d]",fd);
	_base = base;
	_state = State::Connected;
	_local = local;
	_peer = peer;
	_channel = new Channel(base, fd,kReadEvent);
	info("tcp constructed [%s]----->>>>[%s] fd[%d]\n",_local.toString().c_str(),_peer.toString().c_str(),fd);
	TcpConnPtr conn = shared_from_this();
	trace("TcpConnPtr's use_count[%d]",conn.use_count());
	conn->_channel->onRead([=]{conn->handleRead(conn);});//Channel::onRead -> TcpConn::handleRead
	conn->_channel->onWrite([=]{conn->handleWrite(conn);});//Channel::onWrite->TcpConn::handleWrite
	//just for echo test
	onRead([this](const TcpConnPtr&){info("readed contents[%s]",_input.end());});
	onWritable([this](const TcpConnPtr&){info("wrote contents[%s]",_input.end());});
	trace("TcpConnPtr's use_count[%d]",conn.use_count());
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
		info("poll fd[%d] return [%d] revents[%d]\n",_channel->fd(),r,pfd.revents);
		cleanup(conn); 
		return -1;
	}
	return 0;
}
void TcpConn::handleRead(const TcpConnPtr& conn)
{
	info("TcpConn::handleRead TcpConnPtr[%p]",conn);
	while (_state == State::Connected)
	{
		_input.makeRoom();
		int ret = 0;
		if (_channel->fd() >= 0 )
		{
			ret = ::read(_channel->fd(), _input.end(),_input.space());
			info("channel[%lld] fd[%d] readed[%d] bytes",(long long)_channel->id(), _channel->fd(),ret);
		}
		if (ret == -1 && errno == EINTR)
		{
			continue;
		}
		else if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			//no content to read ,so this turn finish,_readcb(conn)
			break;
		}
		else if(_channel->fd() == -1 || ret == 0 || ret == -1) //client quit
		{
			debug("handleRead() ret[%d]",ret);
			cleanup(conn);
		}
		else{
			send(_input.end(),ret); //just for echo test
			if (_writecb)
				_writecb(conn);
			_input.addSize(ret);
		}
	}
}
void TcpConn::handleWrite(const TcpConnPtr& conn)
{

	info("TcpConn::handleWrite TcpConnPtr[%p]",conn);
	if (_state == State::Connected)
	{
		ssize_t sended = iSend(_output.begin(),_output.size());
		_output.consume(sended);
		if (_output.empty() && _writecb)
		{
		}
		if (_output.empty() && _channel->writeEnabled())
		{
			_channel->enableWrite(false);
		}
	}
	else
	{
		error("handle write unexpedted");
	}
}
ssize_t TcpConn::iSend(const char* buf, size_t len)
{
	size_t sended = 0;
	while (len > sended)
	{
		ssize_t wd = ::write(_channel->fd(),buf + sended, len - sended);
		info("channel id[%lld] fd[%d] write[%d] bytes\n",(long long)_channel->id(),
				_channel->fd(),wd);
		if (wd > 0)
		{
			sended += wd;
			continue;
		}
		else if (wd == -1 && errno == EINTR){
			continue;
		}
		else if(wd == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) //cant' write any more now
		{
			if (!_channel->writeEnabled()){
				_channel->enableWrite(true);//notify _poller
			}
			break;
		}
		else
		{
			error("write error: channel id[%lld] fd[%d] wd[%d]\n",(long long)_channel->id(),_channel->fd(),wd);
			break;
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
			info("TcpConn::Send succ[%d] bytes\n",nSend);
		}
	}
	else{
		error("TcpConn::Send channel doesn't exist\n");
	}
}

void TcpConn::send(const char* buf, size_t len)
{
	if (_channel)
	{
		if (_output.empty())
		{
			ssize_t sended = iSend(buf,len);
			buf += sended;
			len -= sended;
		}
		if (len)
		{
			_output.append(buf,len);
		}
	}
	else
	{
		warn("connection[%s]---->>[%s] closed,but still writing[%lu]bytes",_local.toString().c_str(),_peer.toString().c_str(),len);	
	}
}

void TcpConn::cleanup(const TcpConnPtr& conn)
{
	if (_readcb && _input.size())
		_readcb(conn);
	if (_state == State::Connected)
		_state = State::Closed;
	info("tcp closing[%s]---->>[%s] channel's fd[%d]",_local.toString().c_str(),_peer.toString().c_str(),
		_channel ? _channel->fd():-1);
	_readcb = _writecb = nullptr;
	Channel* ch = _channel;
	_channel = NULL;
	delete ch;
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
	info("fd[%d] listening at %s\n",fd,_addr.toString().c_str());
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
		info("TcpServer::handleAccept EventBase addr[%p]\n",b);
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
