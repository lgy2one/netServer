/*****************************
 *Copyright 2020, Li GuoYan.
 *
 *Author: Li GuoYan
******************************/

#include "HttpServer.h"
#include <iostream>
#include <functional>
#include <memory>
#include "TimerManager.h"


HttpServer::HttpServer(EventLoop *loop, const int port, const int iothreadnum, const int workerthreadnum)
    : tcpserver_(loop, port, iothreadnum),
    threadpool_(workerthreadnum)
{
    tcpserver_.SetNewConnCallback(std::bind(&HttpServer::HandleNewConnection, this, std::placeholders::_1));
    tcpserver_.SetMessageCallback(std::bind(&HttpServer::HandleMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcpserver_.SetSendCompleteCallback(std::bind(&HttpServer::HandleSendComplete, this, std::placeholders::_1));
    tcpserver_.SetCloseCallback(std::bind(&HttpServer::HandleClose, this, std::placeholders::_1));
    tcpserver_.SetErrorCallback(std::bind(&HttpServer::HandleError, this, std::placeholders::_1));

    threadpool_.Start(); //工作线程池启动
    TimerManager::GetTimerManagerInstance()->Start(); //HttpServer定时器管理器启动
}

HttpServer::~HttpServer()
{

}

/*新连接，创建相应的应用层会话并设计好定时器，并让连接加入定时器和HTTP会话的管理*/
void HttpServer::HandleNewConnection(const spTcpConnection& sptcpconn)
{
    std::shared_ptr<HttpSession> sphttpsession = std::make_shared<HttpSession>(); //创建应用层会话
    spTimer sptimer = std::make_shared<Timer>(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));
    sptimer->Start();//建立新连接时设置定时器的延时时间并启动定时器

    {
        std::lock_guard <std::mutex> lock(mutex_);
        httpsessionnlist_[sptcpconn] = sphttpsession;
        timerlist_[sptcpconn] = sptimer;
    }
}

/*有消息到达，调整定时器，切换到工作线程解析报文并回送回复报文*/
void HttpServer::HandleMessage(const spTcpConnection& sptcpconn, std::string &msg)
{
    std::shared_ptr<HttpSession> sphttpsession;
    spTimer sptimer;
    {
        //管理HTTP会话和TCP连接定时器
        std::lock_guard <std::mutex> lock(mutex_);
        sphttpsession = httpsessionnlist_[sptcpconn];
        sptimer = timerlist_[sptcpconn];
    }

    //若接收到信息，调整该定时器在时间轮中的位置
    sptimer->Adjust(5000, Timer::TimerType::TIMER_ONCE, std::bind(&TcpConnection::Shutdown, sptcpconn));

    if(threadpool_.GetThreadNum() > 0)
    {
        //线程池处理业务，处理完后投递回本IO线程执行send
        HttpRequestContext httprequestcontext;
        std::string responsecontext;

        //对报文进行解析，把结果存进HTTP请求报文结构体里
        bool result = sphttpsession->PraseHttpRequest(msg, httprequestcontext);
        if(result == false)
        {
            sphttpsession->HttpError(400, "Wrong request!", httprequestcontext, responsecontext);
            sptcpconn->Send(responsecontext);
            return;
        }

        sptcpconn->SetAsyncProcessing(true);  //启用工作线程
        threadpool_.AddTask([=]() {   //把任务添加到工作线程的任务队列里
            std::string responsemsg;
            sphttpsession->HttpProcess(httprequestcontext, responsemsg);//报文解析，根据请求报文得出回复报文

            sptcpconn->Send(responsemsg); //任务已经处理完成，切换回原来的IO线程发送

            if(!sphttpsession->KeepAlive())
            {
                /*短连接，可以告诉框架层数据发完就可以关掉TCP连接，
                  不过这里注释掉，交给客户端主动关闭*/
                //sptcpconn->HandleClose();
            }
        });
    }
    else
    {
        //没有开启业务线程池，业务计算直接在IO线程执行
        HttpRequestContext httprequestcontext;
        std::string responsecontext;
        bool result = sphttpsession->PraseHttpRequest(msg, httprequestcontext);
        if(result == false)
        {
            sphttpsession->HttpError(400, "Bad request", httprequestcontext, responsecontext); //请求报文解析错误，报400
            sptcpconn->Send(responsecontext);
            return;
        }

        sphttpsession->HttpProcess(httprequestcontext, responsecontext);

        sptcpconn->Send(responsecontext);

        if(!sphttpsession->KeepAlive())
        {
            /*短连接，可以告诉框架层数据发完就可以关掉TCP连接，
              不过这里注释掉，交给客户端主动关闭*/
            //sptcpconn->HandleClose();
        }
    }
}

/*发送完成*/
void HttpServer::HandleSendComplete(const spTcpConnection& sptcpconn)
{

}

/*客户端关闭，移除该连接*/
void HttpServer::HandleClose(const spTcpConnection& sptcpconn)
{
    {
        std::lock_guard <std::mutex> lock(mutex_);
        httpsessionnlist_.erase(sptcpconn);
        timerlist_.erase(sptcpconn);
    }
}

/*发生错误，移除该连接*/
void HttpServer::HandleError(const spTcpConnection& sptcpconn)
{
    {
        std::lock_guard <std::mutex> lock(mutex_);
        httpsessionnlist_.erase(sptcpconn);
        timerlist_.erase(sptcpconn);
    }
}

/*服务器启动*/
void HttpServer::Start()
{
    tcpserver_.Start();
}
