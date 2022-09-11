//
// Created by heyjude on 2022/7/17.
//

#ifndef WEB_SERVER_THREAD_POOL_CONFIG_H
#define WEB_SERVER_THREAD_POOL_CONFIG_H

#include <thread>

/* Scheduling algorithms.  */
#define SCHED_OTHER		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#ifdef __USE_GNU
# define SCHED_BATCH		3
# define SCHED_ISO		4
# define SCHED_IDLE		5
# define SCHED_DEADLINE		6

# define SCHED_RESET_ON_FORK	0x40000000
#endif

static const int CGRAPH_CPU_NUM = (int)std::thread::hardware_concurrency();
static const int CGRAPH_THREAD_TYPE_PRIMARY = 1;
static const int CGRAPH_THREAD_TYPE_SECONDARY = 2;
#ifndef _WIN32
static const int CGRAPH_THREAD_SCHED_OTHER = SCHED_OTHER;
static const int CGRAPH_THREAD_SCHED_RR = SCHED_RR;
static const int CGRAPH_THREAD_SCHED_FIFO = SCHED_FIFO;
#else
/** 线程调度策略，暂不支持windows系统 */
static const int CGRAPH_THREAD_SCHED_OTHER = 0;
static const int CGRAPH_THREAD_SCHED_RR = 0;
static const int CGRAPH_THREAD_SCHED_FIFO = 0;
#endif
static const int CGRAPH_THREAD_MIN_PRIORITY = 0;                                            // 线程最低优先级
static const int CGRAPH_THREAD_MAX_PRIORITY = 99;                                           // 线程最高优先级
static const unsigned int CGRAPH_MAX_BLOCK_TTL = 10000000;                                         // 最大阻塞时间，单位为ms
static const int CGRAPH_DEFAULT_TASK_STRATEGY = -1;                                         // 默认线程调度策略


static const int CGRAPH_DEFAULT_THREAD_SIZE = (CGRAPH_CPU_NUM > 0) ? CGRAPH_CPU_NUM : 8;    // 默认主线程个数
static const int CGRAPH_MAX_THREAD_SIZE = (CGRAPH_DEFAULT_THREAD_SIZE * 2);                 // 最大线程个数
#ifndef _WIN32
static const int CGRAPH_MAX_TASK_STEAL_RANGE = 2;                                           // 盗取机制相邻范围
#else
static const int CGRAPH_MAX_TASK_STEAL_RANGE = 0;                                           // windows平台暂不支持任务盗取功能
#endif
static const bool CGRAPH_BATCH_TASK_ENABLE = false;                                         // 是否开启批量任务功能
static const int CGRAPH_MAX_LOCAL_BATCH_SIZE = 2;                                           // 批量执行本地任务最大值
static const int CGRAPH_MAX_POOL_BATCH_SIZE = 2;                                            // 批量执行通用任务最大值
static const int CGRAPH_MAX_STEAL_BATCH_SIZE = 2;                                           // 批量盗取任务最大值
static const bool CGRAPH_FAIR_LOCK_ENABLE = false;                                          // 是否开启公平锁（非必须场景不建议开启，开启后CGRAPH_BATCH_TASK_ENABLE无效）
static const int CGRAPH_SECONDARY_THREAD_TTL = 10;                                          // 辅助线程ttl，单位为s
static const int CGRAPH_MONITOR_SPAN = 5;                                                   // 监控线程执行间隔，单位为s
static const bool CGRAPH_BIND_CPU_ENABLE = true;                                            // 是否开启绑定cpu模式（仅针对主线程）
static const int CGRAPH_PRIMARY_THREAD_POLICY = CGRAPH_THREAD_SCHED_OTHER;                  // 主线程调度策略
static const int CGRAPH_SECONDARY_THREAD_POLICY = CGRAPH_THREAD_SCHED_OTHER;                // 辅助线程调度策略
static const int CGRAPH_PRIMARY_THREAD_PRIORITY = CGRAPH_THREAD_MIN_PRIORITY;               // 主线程调度优先级（取值范围0~99）
static const int CGRAPH_SECONDARY_THREAD_PRIORITY = CGRAPH_THREAD_MIN_PRIORITY;             // 辅助线程调度优先级（取值范围0~99）

struct ThreadPoolConfig{
    /** 具体值含义，参考UThreadPoolDefine.h文件 */
    int default_thread_size_ = CGRAPH_DEFAULT_THREAD_SIZE;
    int max_thread_size_ = CGRAPH_MAX_THREAD_SIZE;
    int max_task_steal_range_ = CGRAPH_MAX_TASK_STEAL_RANGE;
    int max_local_batch_size_ = CGRAPH_MAX_LOCAL_BATCH_SIZE;
    int max_pool_batch_size_ = CGRAPH_MAX_POOL_BATCH_SIZE;
    int max_steal_batch_size_ = CGRAPH_MAX_STEAL_BATCH_SIZE;
    int secondary_thread_ttl_ = CGRAPH_SECONDARY_THREAD_TTL;
    int monitor_span_ = CGRAPH_MONITOR_SPAN;
    int primary_thread_policy_ = CGRAPH_PRIMARY_THREAD_POLICY;
    int secondary_thread_policy_ = CGRAPH_SECONDARY_THREAD_POLICY;
    int primary_thread_priority_ = CGRAPH_PRIMARY_THREAD_PRIORITY;
    int secondary_thread_priority_ = CGRAPH_SECONDARY_THREAD_PRIORITY;
    bool bind_cpu_enable_ = CGRAPH_BIND_CPU_ENABLE;
    bool batch_task_enable_ = CGRAPH_BATCH_TASK_ENABLE;
    bool fair_lock_enable_ = CGRAPH_FAIR_LOCK_ENABLE;

protected:
    /**
     * 计算可盗取的范围，盗取范围不能超过默认线程数-1
     * @return
     */
    [[nodiscard]] int calcStealRange() const {
        int range = std::min(this->max_task_steal_range_, this->default_thread_size_ - 1);
        return range;
    }


    /**
     * 计算是否开启批量任务
     * 开启条件：开关批量开启，并且 未开启非公平锁
     * @return
     */
    [[nodiscard]] bool calcBatchTaskRatio() const {
        bool ratio = (this->batch_task_enable_) && (!this->fair_lock_enable_);
        return ratio;
    }


};

using ThreadPoolConfigPtr = ThreadPoolConfig *;

#endif //WEB_SERVER_THREAD_POOL_CONFIG_H
