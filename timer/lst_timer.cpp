#include "lst_timer.h"
#include "../http/http_conn.h"

sort_timer_lst::~sort_timer_lst()
{
    util_timer *tmp = head;
    while(tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void sort_timer_lst::add_timer(util_timer *timer)
{
    if(timer == nullptr) return;
    if(head == nullptr) {
        head = timer;
        tail = timer;
        return;
    }
    //如果当前节点超时时间小于head，放在head前面
    if(timer->expire < head->expire) {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return ;
    }
    //其他情况，调用私有成员进行节点调整
    add_timer(timer, head);
}

 //调整定时器，任务发生变化时，调整定时器在链表中的位置
void sort_timer_lst::adjust_timer(util_timer *timer)
{
    if(timer == nullptr) return;
    util_timer *tmp = timer->next;
    //timer在链表尾部 || 超时时间仍然小于下一个
    if(tmp == nullptr || (timer->expire < tmp->expire)) return;

    //如果timer在头部，先取出，重新插入
    if(timer == head) {
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }
    else {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

void sort_timer_lst::delete_timer(util_timer *timer)
{
    if(timer == nullptr) return;

    //链表中只有一个定时器
    if(timer == head && timer == tail) {
        delete timer;
        head = nullptr;
        tail = nullptr;
        return ;
    }

    //timer是head节点
    if(timer == head) {
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return ;
    }

    //timer是tail节点
    if(timer == tail) {
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }

    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}

void sort_timer_lst::tick()
{
    if(head == nullptr) return;
    time_t cur = time(nullptr);
    util_timer *tmp = head;
    while(tmp) {
        //升序链表，后面越来越大，如果当前时间小于超时时间，那后面的节点都不会过期
        if(cur < tmp->expire) break;

        //当前时间大于定时器超时时间，执行回调函数
        tmp->cb_func(tmp->user_data);

        //更新头节点
        head = tmp->next;
        if(head != nullptr) {
            head->prev = nullptr;
        }
        delete tmp;
        tmp = head;
    }
}

//私有成员，调整内部节点
//把timer插入到一个升序链表中
void sort_timer_lst::add_timer(util_timer *timer, util_timer *lst_head) 
{
    util_timer *pre = lst_head;
    util_timer *tmp = pre->next;
    while(tmp) {
        if(timer->expire < tmp->expire) {
            pre->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = pre;
            break;
        }
        pre = tmp;
        tmp = tmp->next;
    }
    //遍历结束发现，需要放到尾部
    if(tmp == nullptr) {
        pre->next = timer;
        timer->prev = pre;
        timer->next = nullptr;
        tail = timer;
    }
}

void Utils::init(int timeslot) 
{
    m_TIMESLOT = timeslot;
}

//对文件描述符设置非阻塞
int Utils::setnonblocking(int fd) 
{
    //获取文件描述符旧的设置状态
    int old_option = fcntl(fd, F_GETFL);
    //设置新的状态
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    //返回旧状态，方便恢复设置
    return old_option;
}

//将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRGIMode)
{
    epoll_event event;
    event.data.fd = fd;

    //开启ET模式
    if( TRGIMode == 1) {
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    }
    else {
        event.events = EPOLLIN | EPOLLRDHUP;
    }

    if(one_shot) {
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

//信号处理函数
void Utils::sig_handler(int sig) 
{
    //异步信号处理环境中，信号处理函数可能发生中断并在其他上下文执行，
    //而errno是个全局量，若在信号处理函数中发生错误可能会影响其他调用
    //信号处理函数的线程，所以调用完后恢复原值可以避免这种错误
    int save_errno = errno;
    int msg = sig;

    //将信号值从管道写端写入，传输字符类型，而非整型
    send(u_pipefd[1], (char*)&msg, 1, 0);
    
    errno = save_errno;
}

//设置信号函数,加入监听的信号集
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));

    sa.sa_handler = handler;
    if(restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, nullptr) != -1);
}

//定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler() 
{
    m_time_lst.tick();
    /*设置定时器，经过m_TIMESLOT秒后发送信号*/
    alarm(m_TIMESLOT);
}

void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;


void timer_cb_func(client_data *user_data)
{
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);

    http_conn::m_user_count--;
}