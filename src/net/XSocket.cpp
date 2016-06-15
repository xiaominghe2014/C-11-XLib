//----------------------------------------------//
//  XSocket.cpp
//  xlib
//
//  Copyright (c) __DATA__  Ximena.
//  Created by ximena on 16/6/14.
//    Copyright © 2016年 ximena. All rights reserved.
//  Contact Email: xiaominghe2014@gmail.com
//----------------------------------------------//
#include "XSocket.h"

XLIB_BEGAIN
namespace net {

#define SOCKET_MAX_BUFFER_LEN 1024
    
    SocketException::SocketException(const std::string &message)
    {
        this->_error = message;
    }
    
    SocketException::~SocketException() throw()
    {
    }
    
    const char* SocketException::what() const throw()
    {
        return this->_error.c_str();
    }
    
    std::ostream& operator<< (std::ostream &out, SocketException &e)
    {
        out << e.what();
        return out;
    }
    
    
    ////////////////////XSOCKET//////////////////////
    XSocket::XSocket():mSocket(INVALID_SOCKET),isBlock(false),_running(false),_threadEnd(false)
    {
        
    }
    
    XSocket::~XSocket()
    {
        Clean();
    }
    
    void XSocket::IPV4(ipv4* addrV4, short protocolFamily, uint port, const std::string &addr)
    {
        bzero(addrV4, sizeof(addrV4));
        addrV4->sin_family = protocolFamily;
        addrV4->sin_port = htons(port);
        addrV4->sin_addr.s_addr=inet_addr(addr.c_str());
    }
    
    void XSocket::IPV6(ipv6* addrV6, short protocolFamily, uint port, const in6_addr &addr)
    {
        bzero(addrV6, sizeof(addrV6));
        addrV6->sin6_family = protocolFamily;
        addrV6->sin6_port = htons(port);
        addrV6->sin6_addr= addr;
    }
    
    SOCKET XSocket::Socket(const _socket& aSocket)
    {
        mSocket = socket(aSocket.protocolFamily,aSocket.socketType,aSocket.protocol);
        if(mSocket==INVALID_SOCKET)
        {
            // add handler error code here
            throw SocketException("init socket failed!");
        }
        return mSocket;
    }
    
    
    int32 XSocket::Connect(SOCKET sd, const struct sockaddr *addr, int32 addrLen)
    {
        int32 code = connect(sd,addr,addrLen);
        if(code==SOCKET_ERROR)
        {
            // add handler error code here
            throw SocketException("connect failed!");
        }
        return code;
    }
    
    
    int32 XSocket::Send(const char* buffer, size_t len)
    {
        if(len>SOCKET_MAX_BUFFER_LEN)
        {
            throw SocketException("data length higher then max buffer len!");
        }
        ssize_t ret=0;
        while (len>0)
        {
            if((ret=send(mSocket, buffer, len, 0))==-1)
            {
                if(errno == EINTR) continue;
                throw SocketException("send failed!");
            }
            else if(ret==0) throw SocketException("the socket is closed!");
            len-=ret;
            buffer+=ret;
        }
        return (int32)ret;
    }
    
    
    std::string XSocket::Receive()
    {
        std::string ret="";
        char buf[SOCKET_MAX_BUFFER_LEN];
        while(ret.find("\n")==std::string::npos)
        {
            ssize_t s=0;
            if((s=recv(mSocket, buf, 1024, 0))==-1)
            {
                if(errno == EINTR) continue;
                throw SocketException("receive failed!");
            }
            else if(s==0) throw SocketException("the socket is closed!");
            ret.append(buf,s);
        }
        return ret;
    }
    
    void XSocket::loop()
    {
        fd_set _read_set;
        struct timeval timeout;
        FD_ZERO(&_read_set);
        FD_SET(mSocket,&_read_set);
        _running = true;
        timeout.tv_sec = 0;
        
        /* 
         if your engine is cocos2dx
            0.016 seconds. Wake up once per frame at 60PFS.
         else
            And you can define your timeout depending on your needs  */
        timeout.tv_usec = 16000;
        
        while (!_threadEnd)
        {
            int s=0;
            if((s = select(0,&_read_set,NULL,NULL,&timeout))==-1)
            {
                /* error */
                if(errno != EINTR)
                    throw SocketException("Abnormal error in select()\n");
                continue;
            }
            else if( s == 0 )
            {
                /* timeout. do something ? */
            }
            if(FD_ISSET(mSocket, &_read_set))
            {
                handlerMessage(Receive());
            }
        }
    }
    
    
    int32 XSocket::Close()
    {
        return close(mSocket);
    }
    
    
    int32 XSocket::Clean()
    {
        _running = false;
        return clean();
    }
    
    ///////////////////////TCP//////////////////////
    XSocketTCP::XSocketTCP()
    {
        
    }
    
    XSocketTCP::~XSocketTCP()
    {
        stop();
    }
    
    void XSocketTCP::stop()
    {
        if( _running ) {
            _threadEnd = true;
            if (_thread.joinable())
            {
                _thread.join();
            }
        }
    }
    
    bool XSocketTCP::startClient(const _server& aServer,bool isSync)
    {
        if(!checkHost(aServer.ip)) throw SocketException("error ip!");
        _socket tcpSocket;
        tcpSocket.protocolFamily = AF_INET;
        tcpSocket.socketType = SOCK_STREAM;
        tcpSocket.protocol = IPPROTO_TCP;
        mSocket = Socket(tcpSocket);
        ipv4 addr;
        IPV4((ipv4 *)&addr, AF_INET, aServer.port, std::string(aServer.ip));
        Connect(mSocket, (sockaddr*)&addr, sizeof(addr));
        _running = true;
        if(isSync)
        {
            _thread = std::thread( std::bind( &XSocketTCP::loop, this) );
        }
        return true;
    }
    
    bool XSocketTCP::startServer()
    {
        return false;
    }
    
    
    
    void XSocketTCP::handlerMessage(const std::string &message)
    {
        //add your handler code here
        std::cout<<message;
    }
    
    bool XSocketTCP::checkHost(const std::string &host)
    {
        const char *pChar;
        bool rv = true;
        int tmp1, tmp2, tmp3, tmp4, i;
        while( 1 )
        {
            i = sscanf(host.c_str(), "%d.%d.%d.%d", &tmp1, &tmp2, &tmp3, &tmp4);
            
            if( i != 4 )
            {
                rv = false;
                break;
            }
            if( (tmp1 > 255) || (tmp2 > 255) || (tmp3 > 255) || (tmp4 > 255) )
            {
                rv = false;
                break;
            }
            for( pChar = host.c_str(); *pChar != 0; pChar++ )
            {
                if( (*pChar != '.')
                   && ((*pChar < '0') || (*pChar > '9')) )
                {
                    rv = false;
                    break;
                }
            }
            break;
        }
        
        return rv;
    }
    
    
}
XLIB_END