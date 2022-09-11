//
// Created by heyjude on 2022/7/16.
//

#ifndef WEB_SERVER_THREAD_PRIMARY_H
#define WEB_SERVER_THREAD_PRIMARY_H

#include "../status.h"
#include "thread_base.h"
#include "work_steal_queue.h"

class ThreadPrimary : public ThreadBase {
 public:
  explicit ThreadPrimary() : index_(-1), pool_threads_(nullptr) {
    thread_type_ = ThreadType::PrimaryThread;
  }

  Status init() {
    is_init_ = true;
    thread_ = std::move(std::thread(&ThreadPrimary::run, this));
  }

  /**
   * 注册线程池相关内容，需要在init之前使用
   * @param index
   * @param poolTaskQueue
   * @param poolThreads
   * @param config
   */
  //    Status setThreadPoolInfo(int index,
  //                              UAtomicQueue <UTaskWrapper> *poolTaskQueue,
  //                              std::vector<UThreadPrimary *> *poolThreads,
  //                              UThreadPoolConfigPtr config) {
  //        CGRAPH_FUNCTION_BEGIN
  //                CGRAPH_ASSERT_INIT(false)    // 初始化之前，设置参数
  //        CGRAPH_ASSERT_NOT_NULL(poolTaskQueue)
  //        CGRAPH_ASSERT_NOT_NULL(poolThreads)
  //        CGRAPH_ASSERT_NOT_NULL(config)
  //
  //        this->index_ = index;
  //        this->pool_task_queue_ = poolTaskQueue;
  //        this->pool_threads_ = poolThreads;
  //        this->config_ = config;
  //        CGRAPH_FUNCTION_END
  //    }

  /**
   * 线程执行函数
   * @return
   */
  Status run() {
    //        CGRAPH_FUNCTION_BEGIN
    //                CGRAPH_ASSERT_INIT(true)
    //        CGRAPH_ASSERT_NOT_NULL(pool_threads_)
    //        CGRAPH_ASSERT_NOT_NULL(config_)

    /**
     * 线程池中任何一个primary线程为null都不可以执行
     * 防止线程初始化失败的情况，导致的崩溃
     * 理论不会走到这个判断逻辑里面
     */
    if (std::any_of(pool_threads_->begin(), pool_threads_->end(),
                    [](ThreadPrimary *thd) { return nullptr == thd; })) {
      return Status("primary thread is null");
    }

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
   * 获取并执行任务
   * @return
   */
  void processTask() {
    TaskWrapper task;
    if (popTask(task) || popPoolTask(task) || stealTask(task)) {
      runTask(task);
    } else {
      std::this_thread::yield();
    }
  }

  /**
   * 获取批量执行task信息
   */
  void processTasks() {
    TaskWrapperArr tasks;
    if (popTask(tasks) || popPoolTask(tasks) || stealTask(tasks)) {
      // 尝试从主线程中获取/盗取批量task，如果成功，则依次执行
      runTasks(tasks);
    } else {
      std::this_thread::yield();
    }
  }

  /**
   * 从本地弹出一个任务
   * @param task
   * @return
   */
  bool popTask(TaskWrapperRef task) {
    return work_stealing_queue_.TryPop(task);
  }

  /**
   * 从本地弹出一批任务
   * @param tasks
   * @return
   */
  bool popTask(TaskWrapperArrRef tasks) {
    return work_stealing_queue_.TryPop(tasks, config_->max_local_batch_size_);
  }

  /**
   * 从其他线程窃取一个任务
   * @param task
   * @return
   */
  bool stealTask(TaskWrapperRef task) {
    if (unlikely(pool_threads_->size() < config_->default_thread_size_)) {
      /**
       * 线程池还未初始化完毕的时候，无法进行steal。
       * 确保程序安全运行。
       */
      return false;
    }

    /**
     * 窃取的时候，仅从相邻的primary线程中窃取
     * 待窃取相邻的数量，不能超过默认primary线程数
     */
    int range = config_->calcStealRange();
    for (int i = 0; i < range; i++) {
      /**
       * 从线程中周围的thread中，窃取任务。
       * 如果成功，则返回true，并且执行任务。
       */
      int curIndex = (index_ + i + 1) % config_->default_thread_size_;
      if (nullptr != (*pool_threads_)[curIndex] &&
          ((*pool_threads_)[curIndex])->work_stealing_queue_.TrySteal(task)) {
        return true;
      }
    }

    return false;
  }

  /**
   * 从其他线程盗取一批任务
   * @param tasks
   * @return
   */
  bool stealTask(TaskWrapperArrRef tasks) {
    if (unlikely(pool_threads_->size() < config_->default_thread_size_)) {
      return false;
    }

    int range = config_->calcStealRange();
    for (int i = 0; i < range; i++) {
      int curIndex = (index_ + i + 1) % config_->default_thread_size_;
      if (nullptr != (*pool_threads_)[curIndex] &&
          ((*pool_threads_)[curIndex])
              ->work_stealing_queue_.TrySteal(tasks,
                                              config_->max_steal_batch_size_)) {
        return true;
      }
    }

    return false;
  }

 private:
  int index_{-1};
  WorkStealQueue work_stealing_queue_;
  std::vector<ThreadPrimary *> *pool_threads_;
};

using ThreadPrimaryPtr = ThreadPrimary *;

#endif  // WEB_SERVER_THREAD_PRIMARY_H
