//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_UNBOUND_QUEUE_H
#define WEB_SERVER_UNBOUND_QUEUE_H

#include <mutex>
#include <condition_variable>
#include <queue>

template<typename T>
class UNBoundQueue {
public:
    UNBoundQueue() = default;

    /**
     * 等待弹出
     * @param value
     */
    void WaitPop(T &value) {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [this] { return !queue_.empty(); });
        value = std::move(*queue_.front());
        queue_.pop();
    }

    /**
     * 尝试弹出
     * @param value
     * @return
     */
    bool TryPop(T &value) {
        std::unique_lock<std::mutex> lk(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(*queue_.front());
        queue_.pop();
    }

    /**
     * 尝试弹出多个任务
     * @param values
     * @param maxPoolBatchSize
     * @return
     */
    bool TryPop(std::vector<T> &values, int maxPoolBatchSize) {
        std::unique_lock<std::mutex> lk(mutex_);
        if (queue_.empty() || maxPoolBatchSize <= 0) {
            return false;
        }

        while (!queue_.empty() && maxPoolBatchSize--) {
            values.emplace_back(std::move(*queue_.front()));
            queue_.pop();
        }

        return true;
    }

    /**
     * 阻塞式等待弹出
     * @return
     */
    std::unique_ptr<T> WaitPop() {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [this] { return !queue_.empty(); });
        std::unique_ptr<T> result = std::move(queue_.front());
        queue_.pop();
        return result;
    }

    /**
     * 非阻塞式等待弹出
     * @return
     */
    std::unique_ptr<T> TryPop() {
        std::unique_lock<std::mutex> lk(mutex_);
        if (queue_.empty()) { return std::unique_ptr<T>(); }
        std::unique_ptr<T> result = std::move(queue_.front());
        queue_.pop();
        return result;
    }

    void Push(T &&value) {
//        std::unique_ptr<T> task(std::make_unique<T>(std::move(value)));
        std::unique_lock<std::mutex> lk(mutex_);
        queue_.push(std::forward<T>(value));
        cv_.notify_one();
    }


private:
    std::mutex mutex_;
    std::queue<std::unique_ptr<T>> queue_;
    std::condition_variable cv_;
};

#endif //WEB_SERVER_UNBOUND_QUEUE_H
