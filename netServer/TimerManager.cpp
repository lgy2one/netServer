/*****************************
 *Copyright 2020, Li GuoYan.
 *
 *Author: Li GuoYan
******************************/

#include "TimerManager.h"
#include <iostream>
#include <thread>
#include <ctime>
#include <ratio>
#include <chrono>
#include <unistd.h>
#include <sys/time.h>

//初始化
TimerManager*  TimerManager::ptimermanager_ = nullptr;
std::mutex TimerManager::mutex_;
TimerManager::GC TimerManager::gc;
const int TimerManager::slotinterval = 1; //槽的时间范围
const int TimerManager::slotnum = 1024;  //槽的数量

TimerManager::TimerManager(/* args */)
    : currentslot(0),
    timewheel(slotnum, nullptr),
    running_(false),
    th_()
{

}

TimerManager::~TimerManager()
{
    Stop();
}

/*创建定时器管理器，返回该定时器指针*/
TimerManager* TimerManager::GetTimerManagerInstance()
{
    if(ptimermanager_ == nullptr) //指针为空，则创建管理器
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if(ptimermanager_ == nullptr) //双重检测
        {
            ptimermanager_ = new TimerManager();
        }
    }
    return ptimermanager_;
}

/*线程安全下计算定时器的相关参数并将定时器添加到时间轮*/
void TimerManager::AddTimer(Timer* ptimer)
{
    if(ptimer == nullptr)
        return;
    std::lock_guard<std::mutex> lock(timewheelmutex_);
    CalculateTimer(ptimer);
    AddTimerToTimeWheel(ptimer);
}

/*线程安全下移除定时器*/
void TimerManager::RemoveTimer(Timer* ptimer)
{
    if(ptimer == nullptr)
        return;
    std::lock_guard<std::mutex> lock(timewheelmutex_);
    RemoveTimerFromTimeWheel(ptimer);
}

/*线程安全下修改定时器*/
void TimerManager::AdjustTimer(Timer* ptimer)
{
    if(ptimer == nullptr)
        return;

    std::lock_guard<std::mutex> lock(timewheelmutex_);
    AdjustTimerToWheel(ptimer);
}

/*计算该定时器的相关参数*/
void TimerManager::CalculateTimer(Timer* ptimer)
{
    if(ptimer == nullptr)
        return;

    int tick = 0; //当前超时时间可跨越的槽数量
    int timeout = ptimer->timeout_;
    if(timeout < slotinterval)
    {
        tick = 1; //不足一个slot间隔，按延迟1 slot计算
    }
    else
    {
        tick = timeout / slotinterval;
    }

    ptimer->rotation = tick / slotnum;  //剩下的转数
    int timeslot = (currentslot + tick) % slotnum;//定时器所在的时间槽位置
    ptimer->timeslot = timeslot;
}

/*添加定时器到定时器链表中，每个不同的槽都有一个定时器链表*/
void TimerManager::AddTimerToTimeWheel(Timer* ptimer)
{
    if(ptimer == nullptr)
        return;

    int timeslot = ptimer->timeslot;

    if(timewheel[timeslot])
    {
        ptimer->next = timewheel[timeslot];
        timewheel[timeslot]->prev = ptimer;
        timewheel[timeslot] = ptimer;
    }
    else
    {
        timewheel[timeslot] = ptimer;
    }
}

/*从定时器链表中移除定时器*/
void TimerManager::RemoveTimerFromTimeWheel(Timer* ptimer)
{
    if(ptimer == nullptr)
        return;

    int timeslot = ptimer->timeslot;

    if(ptimer == timewheel[timeslot])
    {
        //头结点
        timewheel[timeslot] = ptimer->next;
        if(ptimer->next != nullptr)
        {
            ptimer->next->prev = nullptr;
        }
        ptimer->prev = ptimer->next = nullptr;
    }
    else
    {
        if(ptimer->prev == nullptr) //不在时间轮的链表中，即已经被删除了
            return;
        ptimer->prev->next = ptimer->next;
        if(ptimer->next != nullptr)
            ptimer->next->prev = ptimer->prev;

        ptimer->prev = ptimer->next = nullptr;
    }
}

/*修改定时器，其实就是先移除再增加*/
void TimerManager::AdjustTimerToWheel(Timer* ptimer)
{
    if(ptimer == nullptr)
        return;

    RemoveTimerFromTimeWheel(ptimer);
    CalculateTimer(ptimer);
    AddTimerToTimeWheel(ptimer);
}


void TimerManager::CheckTimer()//执行当前slot的任务
{
    std::lock_guard<std::mutex> lock(timewheelmutex_);
    Timer *ptimer = timewheel[currentslot];
    while(ptimer != nullptr)
    {
        if(ptimer->rotation > 0)
        {
            --ptimer->rotation;
            ptimer = ptimer->next;
        }
        else
        {
            //可执行定时器任务
            ptimer->timercallback_(); //注意：任务里不能把定时器自身给清理掉！！！我认为应该先移除再执行任务
            if(ptimer->timertype_ == Timer::TimerType::TIMER_ONCE)
            {
                Timer *ptemptimer = ptimer;
                ptimer = ptimer->next;
                RemoveTimerFromTimeWheel(ptemptimer);
            }
            else
            {
                Timer *ptemptimer = ptimer;
                ptimer = ptimer->next;
                AdjustTimerToWheel(ptemptimer);
                if(currentslot == ptemptimer->timeslot && ptemptimer->rotation > 0)
                {
                    //如果定时器多于一转的话，需要先对rotation减1，否则会等待两个周期
                    --ptemptimer->rotation;
                }
            }
        }
    }
    currentslot = (++currentslot) % TimerManager::slotnum; //移动至下一个时间槽
}

void TimerManager::CheckTick()
{
    int si = TimerManager::slotinterval;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int oldtime = (tv.tv_sec % 10000) * 1000 + tv.tv_usec / 1000;
    int time;
    int tickcount;
    while(running_)
    {
        gettimeofday(&tv, NULL);
        time = (tv.tv_sec % 10000) * 1000 + tv.tv_usec / 1000;
        tickcount = (time - oldtime) / slotinterval; //计算两次check的时间间隔占多少个slot
        //oldtime = time;
        oldtime = oldtime + tickcount*slotinterval;

        for(int i = 0; i < tickcount; ++i)
        {
            TimerManager::GetTimerManagerInstance()->CheckTimer();
        }
        std::this_thread::sleep_for(std::chrono::microseconds(500)); //milliseconds(si)时间粒度越小，延时越不精确，因为上下文切换耗时
    }
}

void TimerManager::Start()
{
    running_ = true;
    th_ = std::thread(&TimerManager::CheckTick, this);
}

void TimerManager::Stop()
{
    running_ = false;
    if(th_.joinable())
        th_.join();
}
