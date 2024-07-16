#ifndef LOCKER_H
#define LOCKER_H

//主要是对线程同步机制中使用到的信号量，锁和条件变量进行了封装
//为了对重复的代码封装成函数，减少重复
#include<exception>
#include<pthread.h>
#include<semaphore.h>

//信号量类
class sem 
{
private:
    sem_t m_sem;

public:
    sem() {
        if(sem_init(&m_sem, 0, 0) != 0) {
            throw std::exception();
        }
    }

    sem(int val) {
        if(sem_init(&m_sem, 0, val) != 0) {
            throw std::exception();
        }
    }

    ~sem() {
        sem_destroy(&m_sem);
    }
    //原子操作的方式将信号量值 - 1，若信号量值为0则阻塞
    bool wait() {
        return sem_wait(&m_sem) == 0;
    }
    //原子操作将信号量+1
    bool post() {
        return sem_post(&m_sem) == 0;
    }

};

//锁类
class locker 
{
private:
    pthread_mutex_t m_mutex;

public:
    locker(){
        if(pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }
    //原子操作给互斥锁枷锁，若已锁上会进入阻塞状态
    bool lock() {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    //原子操作给互斥锁解锁
    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    //获取互斥锁指针
    pthread_mutex_t* get() {
        return &m_mutex;
    }
};

//条件变量类
class cond
{
private:
    pthread_cond_t m_cond;

public:
    cond() {
        if(pthread_cond_init(&m_cond, NULL) != 0) {
            throw std::exception();
        }
    }

    ~cond() {
        pthread_cond_destroy(&m_cond);
    }
    //以广播的方式唤醒所有等待目标条件变量的线程
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }
    //以信号的方式唤醒所有等待目标条件变量的线程
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }
    //等待目标条件变量
    bool wait(pthread_mutex_t *m_mutex) {
        return pthread_cond_wait(&m_cond, m_mutex) == 0;
    }
    //设置了超时时间
    bool timedwait(pthread_mutex_t *m_mutex, struct timespec *m_abstime) {
        return pthread_cond_timedwait(&m_cond, m_mutex, m_abstime) == 0;
    }
};



#endif