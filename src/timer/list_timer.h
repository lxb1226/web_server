
/**
*  @DATE: 2021/11/24 23:08
*  @author: heyjude
*  @email: 1944303766@qq.com
*/
//
// Created by heyjude on 2021/11/24.
//

#ifndef WEB_SERVER_LIST_TIMER_H
#define WEB_SERVER_LIST_TIMER_H

#include <functional>
#include <list>
#include <chrono>

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

class TimerNode {
public:
    TimeStamp expires;
    TimeoutCallBack cb;
};

class ListTimer {
public:
    ListTimer() = default;

    ~ListTimer() = default;

    void add(int timeOut, const TimeoutCallBack &cb);

    void adjust(TimerNode *node, int newExpires);

    void doWork(TimerNode *node);

    void clear();

    void tick();

    void pop();

    int GetNextTick();

private:
    std::list<TimerNode> list;
};


#endif //WEB_SERVER_LIST_TIMER_H
