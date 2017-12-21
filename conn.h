#ifndef __CONN_H__
#define __CONN_H__ 

#include "Util.h"
#include "Event_base.h"
#include "net.h"
#include <assert.h>

class TcpConn;
typedef std::shared_ptr<TcpConn> TcpConnPtr;
typedef std::function<void(const TcpConnPtr&)> TcpCallBack;
class TcpConn : public std::enable_shared_from_this<TcpConn>, public noncopyable 
{
	public:
		enum State { Invalid = 1, Handshaking, Connected, Closed,Failed};
		TcpConn(); //we create TcpConnection by static createConnection
		virtual ~TcpConn();
		static TcpConnPtr createConnection(EventBase* base, const std::string& host, short port,int timeout=0,const std::string& localip="")
		{
			TcpConnPtr conn(new TcpConn());
			return conn;
		}
	public:
		EventBase*     _base;
		Channel*       _channel;
		Ip4Addr        _local,  _peer;
		State          _state;
		TcpCallBack    _readcb, _writecb, _statecb;
		TimerId        _timeoutId;
		std::string    _localIp, _destHost;
		int            _dstPort, _connectTimeout, _reconnectInterval;
		int64_t        _connectedTime;
		Buffer         _input;
		Buffer         _output;
		char           _writeBuffer[1024]; //only for test
		void  handleRead(const TcpConnPtr& conn);
		void  handleWrite(const TcpConnPtr& conn);
		void  cleanup(const TcpConnPtr& conn);
		void  connect(EventBase* base, const std::string& host, short port,int timeout, const std::string& localip);
		void  reconnect();
		void  attach(EventBase* base, int fd, Ip4Addr local, Ip4Addr peer);
	//	void  outputRead(const )
		void  onRead(const TcpCallBack& cb){assert(!_readcb);_readcb = cb;}
		void  onWritable(const TcpCallBack& cb) {_writecb = cb;}
		void  onState(const TcpCallBack& cb) {_statecb = cb;}
		void  close();
		int handleHandshake(const TcpConnPtr&);
		void Send(const char* buf,size_t len);//just simulate first
		void send(const char* buf,size_t len);//by _output
		ssize_t iSend(const char* buf, size_t len);

};

class TcpServer;
typedef std::shared_ptr<TcpServer> TcpServerPtr;
class TcpServer : public noncopyable  
{
	public:
		TcpServer(EventBases* base);
		int bind(const std::string&,short);
		static TcpServerPtr startServer(EventBases* bases,const std::string& host, short port, bool reusePort = false);
		~TcpServer() {}
		Ip4Addr getAddr() {return _addr;}
		EventBase* getBase() {return _base;}
		void onConnCreate(const std::function<TcpConnPtr()>& cb){_createcb = cb;};
		void onConnState(const TcpCallBack& cb){_statecb = cb;}

	private:
		EventBase*      _base;
		EventBases*     _bases;
		Channel*        _listen_channel;
		Ip4Addr         _addr;
		TcpCallBack     _statecb , _readcb;
		std::function<TcpConnPtr()>  _createcb;
		void handleAccept();
};


#endif
