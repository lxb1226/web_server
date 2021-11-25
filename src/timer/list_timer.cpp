
/**
*  @DATE: 2021/11/24 23:08
*  @author: heyjude
*  @email: 1944303766@qq.com
*/
//
// Created by heyjude on 2021/11/24.
//

#include "list_timer.h"

void ListTimer::adjust(TimerNode *node, int newExpires) {

}

// 添加一个结点
void ListTimer::add(int timeOut, const TimeoutCallBack &cb) {
    // 在整个链表中寻找合适的位置插入进去
    TimeStamp time = Clock::now() + MS(timeOut);

    for (auto &it: list) {
        if (it.expires > time) {
            list.insert(it, new TimerNode(time, cb));
        }
    }
}

void ListTimer::doWork(TimerNode *node) {

}

void ListTimer::clear() {

}

void ListTimer::tick() {

}

void ListTimer::pop() {

}

int ListTimer::GetNextTick() {
    return 0;
}
