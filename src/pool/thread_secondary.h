//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_THREAD_SECONDARY_H
#define WEB_SERVER_THREAD_SECONDARY_H

#include "../status.h"
#include "thread_base.h"

class ThreadSecondary : public ThreadBase {
 public:
  explicit ThreadSecondary() {
    cur_ttl_ = 0;
    thread_type_ = ThreadType::PrimaryThread;
  }

 protected:
  Status init() {
    //        CGRAPH_FUNCTION_BEGIN
    //                CGRAPH_ASSERT_INIT(false)
    //        CGRAPH_ASSERT_NOT_NULL(config_)

    cur_ttl_ = config_->secondary_thread_ttl_;
    is_init_ = true;
    thread_ = std::move(std::thread(&ThreadSecondary::run, this));
    //        setSchedParam();
    //        CGRAPH_FUNCTION_END
  }

  /**
   * 设置pool的信息
   * @param poolTaskQueue
   * @param config
   * @return
   */
  Status setThreadPoolInfo(UNBoundQueue<TaskWrapper> *poolTaskQueue,
                           ThreadPoolConfigPtr config) {
    CGRAPH_FUNCTION_BEGIN
    CGRAPH_ASSERT_INIT(false)  // 初始化之前，设置参数
    CGRAPH_ASSERT_NOT_NULL(poolTaskQueue)
    CGRAPH_ASSERT_NOT_NULL(config)

    this->pool_task_queue_ = poolTaskQueue;
    this->config_ = config;
    CGRAPH_FUNCTION_END
  }

  Status run() {
    //        CGRAPH_FUNCTION_BEGIN
    //                CGRAPH_ASSERT_INIT(true)
    //        CGRAPH_ASSERT_NOT_NULL(config_)

    if (config_->calcBatchTaskRatio()) {
      while (done_) {
        processTasks();  // 批量任务获取执行接口
      }
    } else {
      while (done_) {
        processTask();  // 单个任务获取执行接口
      }
    }

    //        CGRAPH_FUNCTION_END
  }

  /**
   * 任务执行函数，从线程池的任务队列中获取信息
   */
  void processTask() {
    TaskWrapper task;
    if (popPoolTask(task)) {
      runTask(task);
    } else {
      std::this_thread::yield();
    }
  }

  /**
   * 批量执行n个任务
   */
  void processTasks() {
    TaskWrapperArr tasks;
    if (popPoolTask(tasks)) {
      runTasks(tasks);
    } else {
      std::this_thread::yield();
    }
  }

  /**
   * 判断本线程是否需要被自动释放
   * @return
   */
  bool freeze() {
    if (likely(is_running_)) {
      cur_ttl_++;
      cur_ttl_ = std::min(cur_ttl_, config_->secondary_thread_ttl_);
    } else {
      cur_ttl_--;  // 如果当前线程没有在执行，则ttl-1
    }

    return cur_ttl_ <= 0;
  }

 private:
  int cur_ttl_;  // 当前最大生存周期

  friend class ThreadPool;
};

using ThreadSecondaryPtr = ThreadSecondary *;

#endif  // WEB_SERVER_THREAD_SECONDARY_H
