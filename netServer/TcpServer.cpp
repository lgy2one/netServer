/*****************************
 *Copyright 2020, Li GuoYan.
 *
 *Author: Li GuoYan
******************************/

#include "TcpServer.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>

void Setnonblocking(int fd);

TcpServer::TcpServer(EventLoop* loop, const int port, const int threadnum)
    : serversocket_(),
    loop_(loop),
    serverchannel_(),
    conncount_(0),
    eventloopthreadpool(loop, threadnum)
{
    //serversocket_.SetSocketOption();
    serversocket_.SetReuseAddr();
    serversocket_.BindAddress(port);
    serversocket_.Listen();
    serversocket_.Setnonblocking();

    serverchannel_.SetFd(serversocket_.fd());//OnNewConnection包含接受套接字的accept函数
    serverchannel_.SetReadHandle(std::bind(&TcpServer::OnNewConnection, this));
    serverchannel_.SetErrorHandle(std::bind(&TcpServer::OnConnectionError, this));
}

TcpServer::~TcpServer()
{

}

void TcpServer::Start()
{
    eventloopthreadpool.Start();

    serverchannel_.SetEvents(EPOLLIN | EPOLLET);
    loop_->AddChannelToPoller(&serverchannel_);
}

/*新TCP连接处理，核心功能，业务功能注册，任务分发*/
void TcpServer::OnNewConnection()
{
    //循环调用accept，获取所有的建立好连接的客户端fd
    struct sockaddr_in clientaddr;
    int clientfd;
    while( (clientfd = serversocket_.Accept(clientaddr)) > 0)
    {
        std::cout << "New client from IP:" << inet_ntoa(clientaddr.sin_addr)
            << ":" << ntohs(clientaddr.sin_port) << std::endl;

        if(++conncount_ >= MAXCONNECTION)//若连接数大于最大连接数，关闭客户端
        {
            close(clientfd);
            continue;
        }
        Setnonblocking(clientfd);//设置客户端为非阻塞

        //轮询调度选择IO线程loop
        EventLoop *loop = eventloopthreadpool.GetNextLoop();

        //创建连接，注册业务函数，当连接有读写等操作时，皆回调到此处，再回调到http的读写函数
        std::shared_ptr<TcpConnection> sptcpconnection = std::make_shared<TcpConnection>(loop, clientfd, clientaddr);
        sptcpconnection->SetMessaeCallback(messagecallback_);
        sptcpconnection->SetSendCompleteCallback(sendcompletecallback_);
        sptcpconnection->SetCloseCallback(closecallback_);
        sptcpconnection->SetErrorCallback(errorcallback_);
        sptcpconnection->SetConnectionCleanUp(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tcpconnlist_[clientfd] = sptcpconnection; //线程安全下将多个客户端连接保存
        }

        newconnectioncallback_(sptcpconnection);//即回调到HTTP的HandleNewConnection，处理新连接

        //做好一切准备工作后添加事件到epoll
        sptcpconnection->AddChannelToLoop();
    }
}

//连接清理,bugfix:这里应该由主loop来执行，投递回主线程删除 OR 多线程加锁删除
void TcpServer::RemoveConnection(std::shared_ptr<TcpConnection> sptcpconnection)
{
    std::lock_guard<std::mutex> lock(mutex_);
    --conncount_;
    //std::cout << "clean up connection, conncount is" << conncount_ << std::endl;
    tcpconnlist_.erase(sptcpconnection->fd());
}

void TcpServer::OnConnectionError()
{
    std::cout << "UNKNOWN EVENT" << std::endl;
    serversocket_.Close();
}

void Setnonblocking(int fd)
{
    int opts = fcntl(fd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(fd,GETFL)");
        exit(1);
    }
    if (fcntl(fd, F_SETFL, opts | O_NONBLOCK) < 0)
    {
        perror("fcntl(fd,SETFL,opts)");
        exit(1);
    }
}
