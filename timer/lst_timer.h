#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <time.h>
#include "../log/log.h"

//定时器类的前置声明，在client-data中需要用到
class util_timer;

//用户数据结构体（连接资源）
struct client_data {
    sockaddr_in address; //客户端的socket地址
    int sockfd;          //socket文件描述符
    util_timer *timer;  //定时器
};

class util_timer {
public:
    util_timer():prev(nullptr), next(nullptr) {}


    time_t expire;  //超时时间
    //回调函数声明：声明一个返回值为空的函数指针cb_func,传入clent_data指针作为函数参数
    void (*cb_func) (client_data *);    //回调函数
    client_data *user_data;     //连接资源

    //双向链表指针
    util_timer *prev;
    util_timer *next;
};

class sort_timer_lst {
private:
    util_timer *head;
    util_timer *tail;
    void add_timer(util_timer *timer, util_timer *lst_head);
public:
    sort_timer_lst() : head(nullptr), tail(nullptr){}
    ~sort_timer_lst();

    void add_timer(util_timer *timer);      //添加定时器，内部调用私有成员
    void adjust_timer(util_timer *timer);   //调整定时器
    void delete_timer(util_timer *timer);    //删除定时器
    void tick();                            //处理定时任务
};


class Utils {
public:
    static int *u_pipefd;
    sort_timer_lst m_time_lst;
    static int u_epollfd;
    int m_TIMESLOT;
public:
    Utils() {}
    ~Utils() {}

    void init(int timeslot) ;

    //设置文件描述符为非阻塞状态 
    int setnonblocking(int fd);
    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRGIMode) ;
    //信号处理函数
    static void sig_handler(int sig);
    //设置信号处理函数
    void addsig(int sig, void(handler)(int), bool restart = true);
    //定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);
};

#endif