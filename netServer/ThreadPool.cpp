/*****************************
 *Copyright 2020, Li GuoYan.
 *
 *Author: Li GuoYan
 *
 *ThreadPool类，简易线程池实现，表示worker线程,执行通用任务线程
******************************/
#include "ThreadPool.h"
#include <deque>
#include <unistd.h>

ThreadPool::ThreadPool(int threadnum)
    : started_(false),
    threadnum_(threadnum),
    threadlist_(),
    taskqueue_(),
    mutex_(),
    condition_()
{

}

ThreadPool::~ThreadPool()
{
    std::cout << "Clean up the ThreadPool " << std::endl;
    Stop();
    for(int i = 0; i < threadnum_; ++i)
    {
        threadlist_[i]->join();
    }
    for(int i = 0; i < threadnum_; ++i)
    {
        delete threadlist_[i];
    }
    threadlist_.clear();
}

void ThreadPool::Start()
{
    if(threadnum_ > 0)
    {
        started_ = true;
        for(int i = 0; i < threadnum_; ++i)
        {
            std::thread *pthread = new std::thread(&ThreadPool::ThreadFunc, this);
            threadlist_.push_back(pthread);
        }
    }
    else
    {
        ;
    }
}

void ThreadPool::Stop()
{
    started_ = false;
    condition_.notify_all();
}

void ThreadPool::AddTask(Task task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        taskqueue_.push(task);
    }
    condition_.notify_one();//依次唤醒等待队列的线程
}

void ThreadPool::ThreadFunc()
{
    std::thread::id tid = std::this_thread::get_id();
    std::stringstream sin;
    sin << tid;
    std::cout << "worker thread is running :" << tid << std::endl;
    Task task;
    while(started_)
    {
        task = NULL;
        {
            std::unique_lock<std::mutex> lock(mutex_);//unique_lock支持解锁又上锁情况
            while(taskqueue_.empty() && started_)
            {
                condition_.wait(lock);
            }
            if(!started_)
            {
                break;
            }

            task = taskqueue_.front();
            taskqueue_.pop();
        }
        if(task)
        {
            try
            {
                task();
            }
            catch (std::bad_alloc& ba)
            {
                std::cerr << "bad_alloc caught in ThreadPool::ThreadFunc task: " << ba.what() << '\n';
                while(1);
            }
        }
    }
}
