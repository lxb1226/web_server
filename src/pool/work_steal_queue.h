//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_WORK_STEAL_QUEUE_H
#define WEB_SERVER_WORK_STEAL_QUEUE_H

#include <deque>
#include <mutex>
#include <thread>

#include "task_warpper.h"

class WorkStealQueue {
public:
    WorkStealQueue() = default;

    void Push(TaskWrapper &&task) {
        while (true) {
            if (mutex_.try_lock()) {
                queue_.emplace_front(std::move(task));
                mutex_.unlock();
                break;
            } else {
                std::this_thread::yield();
            }
        }
    }

    bool TryPop(TaskWrapper &task) {
        bool result = false;
        if (mutex_.try_lock()) {
            if (!queue_.empty()) {
                task = std::move(queue_.front());
                queue_.pop_front();
                result = true;
            }
            mutex_.unlock();
        }

        return result;
    }

    bool TryPop(TaskWrapperArrRef &tasks, int maxLocalBatchSize) {
        bool result = false;
        if (mutex_.try_lock()) {
            while (!queue_.empty() && maxLocalBatchSize--) {
                tasks.emplace_back(std::move(queue_.front()));
                queue_.pop_front();
                result = true;
            }
            mutex_.unlock();
        }

        return result;
    }

    /**
     * 窃取节点，从尾部进行
     * @param task
     * @return
     */
    bool TrySteal(TaskWrapper &task) {
        bool result = false;
        if (mutex_.try_lock()) {
            if (!queue_.empty()) {
                task = std::move(queue_.back());    // 从后方窃取
                queue_.pop_back();
                result = true;
            }
            mutex_.unlock();
        }

        return result;
    }

    /**
     * 批量窃取节点，从尾部进行
     * @param taskArr
     * @return
     */
    bool TrySteal(TaskWrapperArrRef tasks, int maxStealBatchSize) {
        bool result = false;
        if (mutex_.try_lock()) {
            while (!queue_.empty() && maxStealBatchSize--) {
                tasks.emplace_back(std::move(queue_.back()));
                queue_.pop_back();
                result = true;
            }
            mutex_.unlock();
        }

        return result;    // 如果非空，表示盗取成功
    }


private:
    std::deque<TaskWrapper> queue_;
    std::mutex mutex_;
};

#endif //WEB_SERVER_WORK_STEAL_QUEUE_H
