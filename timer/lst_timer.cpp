#include "lst_timer.h"

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