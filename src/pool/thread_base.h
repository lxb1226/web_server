//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_THREAD_BASE_H
#define WEB_SERVER_THREAD_BASE_H

#include <thread>

#include "task_warpper.h"
#include "unbound_queue.h"

enum class ThreadType { PrimaryThread = 0, SecondaryThread = 1 };

class ThreadBase {
 public:
  explicit ThreadBase()
      : done_(true),
        is_init_(false),
        is_running_(false),
        pool_task_queue_(nullptr),
        total_task_num_(0),
        thread_type_(ThreadType::SecondaryThread),
        max_pool_batch_size_(10) {}

  ~ThreadBase() { reset(); }

  virtual bool popPoolTask(TaskWrapperRef task) {
    return (pool_task_queue_ && pool_task_queue_->TryPop(task));
  }

  virtual bool popPoolTask(TaskWrapperArrRef tasks) {
    return (pool_task_queue_ &&
            pool_task_queue_->TryPop(tasks, max_pool_batch_size_));
  }

  void runTask(TaskWrapper &task) {
    is_running_ = true;
    task();
    total_task_num_++;
    is_running_ = false;
  }

  void runTasks(TaskWrapperArr &tasks) {
    is_running_ = true;
    for (auto &task : tasks) {
      task();
    }
    total_task_num_ += tasks.size();
    is_running_ = false;
  }

  void reset() {
    done_ = false;
    if (thread_.joinable()) {
      thread_.join();  // 等待线程结束
    }
    is_init_ = false;
    is_running_ = false;
    total_task_num_ = 0;
  }

 protected:
  bool done_;
  bool is_init_;
  bool is_running_;
  ThreadType thread_type_;
  int max_pool_batch_size_;
  unsigned long long total_task_num_;

  UNBoundQueue<TaskWrapper> *pool_task_queue_;
  std::thread thread_;
};

#endif  // WEB_SERVER_THREAD_BASE_H
