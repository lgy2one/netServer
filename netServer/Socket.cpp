/*****************************
 *Copyright 2020, Li GuoYan.
 *
 *Author: Li GuoYan
******************************/
#include "Socket.h"
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <cstring>

Socket::Socket(/* args */)
{
    serverfd_ = socket(AF_INET, SOCK_STREAM, 0);   //创建一个套接字
    if(-1 == serverfd_)
    {
        perror("socket create fail!");
        exit(-1);
    }
    std::cout << "server create socket" << serverfd_ << std::endl;
}

Socket::~Socket()
{
    close(serverfd_);
    std::cout << "server close..." << std::endl;
}

void Socket::SetSocketOption()
{
    ;
}

void Socket::SetReuseAddr()
{
    int on = 1;         //设置该地址可被重用
    setsockopt(serverfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
}

void Socket::Setnonblocking()
{
    int opts = fcntl(serverfd_, F_GETFL);
    if (opts<0)
    {
        perror("fcntl(serverfd_,GETFL)");
        exit(1);
    }
    if (fcntl(serverfd_, F_SETFL, opts | O_NONBLOCK) < 0)//设置为非阻塞，没有数据时进入下一个循环
    {
        perror("fcntl(serverfd_,SETFL,opts)");
        exit(1);
    }
    std::cout << "server setnonblocking..." << std::endl;
}

bool Socket::BindAddress(int serverport)
{
    struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;   //地址族
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);//表示本机的所有接口的IP都存到s_addr
	serveraddr.sin_port = htons(serverport);
	int resval = bind(serverfd_, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (resval == -1)            //把地址等信息绑定在该套接字上，即socket监听本机所有IP
	{
		close(serverfd_);
		perror("error bind");
		exit(1);
	}
    std::cout << "server bindaddress..." << std::endl;
    return true;
}

bool Socket::Listen()
{
	if (listen(serverfd_, 8192) < 0)  //使创建的套接字被动连接，队列数最大为8192
	{
		perror("error listen");
		close(serverfd_);
		exit(1);
	}
    std::cout << "server listenning..." << std::endl;
    return true;
}

int Socket::Accept(struct sockaddr_in &clientaddr)
{
    socklen_t lengthofsockaddr = sizeof(clientaddr);
    int clientfd = accept(serverfd_, (struct sockaddr*)&clientaddr, &lengthofsockaddr);
    if (clientfd < 0)
    {
        //perror("error accept");
        if(errno == EAGAIN)
            return 0;
		//std::cout << "error accept:there is no new connection accept..." << std::endl;
        return clientfd;
	}
    //std::cout << "server accept,clientfd: " << clientfd << std::endl;
    return clientfd;
}

bool Socket::Close()
{
    close(serverfd_);
    std::cout << "server close..." << std::endl;
    return true;
}

