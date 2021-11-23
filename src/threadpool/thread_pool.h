
/**
*  @DATE: 2021/11/23 19:30
*  @author: heyjude
*  @email: 1944303766@qq.com
*/
//
// Created by heyjude on 2021/11/23.
//

#ifndef WEB_SERVER_THREAD_POOL_H
#define WEB_SERVER_THREAD_POOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <thread>

/**
 * 使用C++11实现的线程池
 *
 */

class ThreadPool {
public:
    explicit ThreadPool(size_t threadNum = 8) : pool_(std::make_shared<Pool>()) {
        for (size_t i = 0; i < threadNum; ++i) {
            std::thread([pool = pool_] {
                std::unique_lock<std::mutex> locker(pool->mtx_);
                while (true) {
                    if (!pool->tasks.empty()) {
                        auto task = std::move(pool->tasks.front());
                        pool->tasks.pop();
                        locker.unlock();
                        task();
                        locker.lock();
                    } else if (pool->isClosed) {
                        break;
                    } else {
                        pool->cond.wait(locker);
                    }
                }
            }).detach();
        }
    };

    ThreadPool() = default;

    ThreadPool(ThreadPool &&) = default;

    ~ThreadPool() {
        if (static_cast<bool>(pool_)) {
            {
                std::lock_guard<std::mutex> locker(pool_->mtx_);
                pool_->isClosed = true;
            }
            pool_->cond.notify_all();
        }
    }

    template<class F>
    void AddTask(F &&task) {
        {
            std::lock_guard<std::mutex> locker(pool_->mtx_);
            pool_->tasks.emplace(std::forward<F>(task));
        }
        pool_->cond.notify_one();
    }

private:
    struct Pool {
        std::mutex mtx_;
        std::condition_variable cond;
        std::queue<std::function<void()>> tasks;
        bool isClosed;
    };

    std::shared_ptr<Pool> pool_;
};

#endif //WEB_SERVER_THREAD_POOL_H
