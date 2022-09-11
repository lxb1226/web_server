//
// Created by heyjude on 2022/7/17.
//

#ifndef WEB_SERVER_TASK_GROUP_H
#define WEB_SERVER_TASK_GROUP_H

#include<functional>
#include <vector>
#include "../status.h"

using DEFAULT_FUNCTION = std::function<void()>;
using DEFAULT_CONST_FUNCTION_REF = const std::function<void()>&;
using CSTATUS_FUNCTION = std::function<Status()>;
using CSTATUS_CONST_FUNCTION_REF = const std::function<Status()>&;
using CALLBACK_FUNCTION = std::function<void(Status)>;
using CALLBACK_CONST_FUNCTION_REF = const std::function<void(Status)>&;

static const unsigned int MAX_BLOCK_TTL = 10000000U;

class TaskGroup{
public:
    explicit TaskGroup() = default;
//    CGRAPH_NO_ALLOWED_COPY(UTaskGroup)

    /**
     * 直接通过函数来申明taskGroup
     * @param task
     * @param ttl
     * @param onFinished
     */
    explicit TaskGroup(DEFAULT_CONST_FUNCTION_REF task,
                        unsigned int ttl = MAX_BLOCK_TTL,
                        CALLBACK_CONST_FUNCTION_REF onFinished = nullptr) noexcept {
        this->addTask(task)
                ->setTtl(ttl)
                ->setOnFinished(onFinished);
    }

    /**
     * 添加一个任务
     * @param task
     */
    TaskGroup* addTask(DEFAULT_CONST_FUNCTION_REF task) {
        task_arr_.emplace_back(task);
        return this;
    }

    /**
     * 设置任务最大超时时间
     * @param ttl
     */
    TaskGroup* setTtl(unsigned int ttl) {
        this->ttl_ = ttl;
        return this;
    }

    /**
     * 设置执行完成后的回调函数
     * @param onFinished
     * @return
     */
    TaskGroup* setOnFinished(CALLBACK_CONST_FUNCTION_REF onFinished) {
        this->on_finished_ = onFinished;
        return this;
    }

    /**
     * 获取最大超时时间信息
     * @return
     */
    [[nodiscard]] unsigned int getTtl() const {
        return this->ttl_;
    }

    /**
     * 清空任务组
     */
    void clear() {
        task_arr_.clear();
    }

    /**
     * 获取任务组大小
     * @return
     */
    [[nodiscard]] size_t getSize() const {
        auto size = task_arr_.size();
        return size;
    }

private:
    std::vector<DEFAULT_FUNCTION> task_arr_;         // 任务消息
    unsigned int ttl_ = MAX_BLOCK_TTL;                      // 任务组最大执行耗时(如果是0的话，则表示不阻塞)
    CALLBACK_FUNCTION on_finished_ = nullptr;        // 执行函数任务结束

    friend class ThreadPool;
};

using TaskGroupPtr = TaskGroup *;
using TaskGroupRef = TaskGroup &;

#endif //WEB_SERVER_TASK_GROUP_H
