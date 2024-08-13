#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../CGImysql/sql_connection_pool.h"

template<typename T>
class threadpool
{
private:
    int m_thread_number;    //线程池中的线程数
    pthread_t *m_threads;   //线程池数组
    int m_max_requests;     //请求队列中允许的最大请求
    std::list<T *> m_workqueue; //请求队列
    locker m_queuelock;     //请求队列的互斥锁
    sem m_queuestat;        //是否有任务需要处理
    connection_pool *m_connPool; //数据库连接池
    int m_actor_model;      //模型切换

public:
    threadpool(int actor_model, connection_pool *connPool, int thread_number = 8, int max_requests = 10000);
    ~threadpool();

    //向队列中添加请求
    bool append(T *request, int stat);
    bool append_p(T *request);
private:
    //工作线程的运行函数
    //从队列中取出任务执行
    static void *worker(void *arg);
    void run();
};

template<typename T>
threadpool<T>::threadpool(int actor_model, connection_pool *connPool, int thread_number, int max_requests) : m_actor_model(actor_model), m_connPool(connPool), m_thread_number(thread_number), m_max_requests(max_requests), m_threads(nullptr)
{
    if(thread_number < 0 || max_requests <= 0) {
        throw std::exception();
    }

    //初始线程数组
    m_threads = new pthread_t[m_thread_number];
    if(!m_threads) {
        throw std::exception();
    }

    for(int i = 0; i < thread_number; i++) {
        //循环创建线程，并将工作线程按要求进行运行
        if(pthread_create(m_threads + i, nullptr, worker, this) != 0) {
            delete []m_threads;
            throw std::exception();
        }

        //将线程进行分离后，不用单独对工作线程进行回收
        if(pthread_detach(m_threads[i]) != 0) {
            delete []m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete []m_threads;
}

template<typename T>
bool threadpool<T>::append(T *request, int stat)
{
    m_queuelock.lock();
    if(m_workqueue.size() >= m_max_requests) {
        m_queuelock.unlock();
        return false;
    }
    //在本项目中，request通常是http数据类型，而m_state则表示该request是读事件还是写事件
    request->m_state = stat;
    m_workqueue.push_back(request);
    m_queuelock.unlock();
    m_queuestat.post();
    return true;
}

template<typename T>
bool threadpool<T>::append_p(T *request)
{
    m_queuelock.lock();
    if(m_workqueue.size() >= m_max_requests) {
        m_queuelock.unlock();
        return false;
    }
    //在本项目中，request通常是http数据类型，而m_state则表示该request是读事件还是写事件
    m_workqueue.push_back(request);
    m_queuelock.unlock();
    m_queuestat.post();
    return true;
}

template<typename T> 
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = static_cast<threadpool*>(arg);
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while(true){
        m_queuestat.wait();
        m_queuelock.lock();
        if(m_workqueue.empty()) {
            m_queuelock.unlock();
            continue;
        }
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelock.unlock();
        if(!request) {
            continue;
        }
        if(m_actor_model == 1) {
            /*代表读事件*/
            if(request->m_state == 0) {
                if(request->read_once()) {
                    request->improv = 1;
                    connectionRAII mysqlcon(&request->mysql, m_connPool);
                    request->process();
                }
                else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
            else {
                /*代表写事件*/
                if(request->write()) {
                    request->improv = 1;
                }
                else {
                    request->improv = 1;
                    request->timer_flag = 1;
                }
            }
        }
        else {
            connectionRAII mysqlcon(&request->mysql, m_connPool);
            request->process();
        }
    }
}
#endif