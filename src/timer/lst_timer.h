
/**
*  @DATE: 2021/10/23 20:59
*  @author: heyjude
*  @email: 1944303766@qq.com
*/
//
// Created by heyjude on 2021/10/23.
//

#ifndef WEB_SERVER_LST_TIMER_H
#define WEB_SERVER_LST_TIMER_H

#include <ctime>
#include <netinet/in.h>
#include "../logger.h"

#define BUFFER_SIZE 64

class util_timer;

struct client_data {
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    util_timer *timer;
};

class util_timer {
public:
    util_timer() : prev(nullptr), next(nullptr) {}

public:
    time_t expire;

    void (*cb_func)(client_data *);

    client_data *user_data;
    util_timer *prev;
    util_timer *next;
};

class sort_timer_lst {
public:
    sort_timer_lst() : tail(nullptr) {
        head = new util_timer();
    }

    ~sort_timer_lst() {
        util_timer *tmp;
        while (head != nullptr) {
            tmp = head->next;
            delete head;
            head = tmp;
        }
    }

    void add_timer(util_timer *timer) {
        if (timer == nullptr) {
            return;
        }

        util_timer *cur = head->next;
        util_timer *pre = head;
        while (cur != nullptr) {
            if (cur->expire < timer->expire) {
                cur = cur->next;
                pre = pre->next;
            } else {
                break;
            }
        }

        timer->next = cur;
        pre->next = timer;
        timer->prev = pre;
        // 说明timer应该放到最后一个
        if (timer->next == nullptr) {
            tail = timer;
        }

    }

    void adjust_timer(util_timer *timer) {
        if (timer == nullptr) {
            return;
        }
        if (head->next == nullptr) {
            return;
        }

        // 调整
        util_timer *next = timer->next;
        // 如果timer是最后一个，或者调整后过期时间仍小于后一个，则不用调整
        while (timer == nullptr || (timer->expire < next->expire)) {
            return;
        }
        // 将timer从链表中拿出来，再加入到链表中
        timer->next->prev = timer->prev;
        timer->prev->next = timer->next;
        add_timer(timer);
    }

    void del_timer(util_timer *timer) {
        if (timer == nullptr) {
            return;
        }
        if (head->next == nullptr) {
            return;
        }
        // 如果链表中只有一个元素
        if ((timer == head->next) && (timer == tail)) {
            head->next = nullptr;
            tail = nullptr;
            delete timer;
            return;
        }

        if (timer == tail) {
            // timer是最后一个
            tail = timer->prev;
        } else {
            timer->next->prev = timer->prev;
            timer->prev->next = timer->next;
        }
        delete timer;
    }

    void tick() {
        if (head->next == nullptr) {
            return;
        }
        LOG_INFO("timer tick \n");
        time_t cur_time = time(nullptr);
        util_timer *tmp = head->next;
        while (tmp != nullptr) {
            if (cur_time < tmp->expire) {
                break;
            }
            // 执行回调函数
            tmp->cb_func(tmp->user_data);
            head->next = tmp->next;
            tmp = head->next;
        }

    }

private:

    util_timer *head;   // head作为头节点，不存储内容，方便后续的操作
    util_timer *tail;
};


#endif //WEB_SERVER_LST_TIMER_H
