//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_TASK_WARPPER_H
#define WEB_SERVER_TASK_WARPPER_H

#include <vector>
#include <memory>
#include "../common.h"

class TaskWrapper {
    struct taskBased {
        virtual void call() = 0;

        virtual ~taskBased() = default;
    };

    template<typename F>
    struct taskDerided : taskBased {
        F func_;

        explicit taskDerided(F &&func) : func_(std::move(func)) {}

        void call() override { func_(); }
    };

    std::unique_ptr<taskBased> impl_ = nullptr;

public:
    template<typename F>
    TaskWrapper(F &&f) : impl_(new taskDerided<F>(std::forward<F>(f))) {
    }

    void operator()() {
        if (likely(impl_)) {
            impl_->call();
        }
    }

    TaskWrapper() = default;

    // 移动构造
    TaskWrapper(TaskWrapper &&task) noexcept: impl_(std::move(task.impl_)) {

    }

    // 移动赋值
    TaskWrapper &operator=(TaskWrapper &&task) noexcept {
        impl_ = std::move(task.impl_);
        return *this;
    }

    // TODO:拷贝赋值 拷贝构造 删除
    TaskWrapper(const TaskWrapper &) = delete;

    TaskWrapper(TaskWrapper &) = delete;

    TaskWrapper &operator=(const TaskWrapper &) = delete;
};

using TaskWrapperRef = TaskWrapper &;
using TaskWrapperPtr = TaskWrapper *;
using TaskWrapperArr = std::vector<TaskWrapper>;
using TaskWrapperArrRef = std::vector<TaskWrapper> &;


#endif //WEB_SERVER_TASK_WARPPER_H
