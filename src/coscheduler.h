#pragma once

#include <list>
#include <mutex>
#include <thread>
#include <functional>

#include "coexecutor.h"
#include "thread.h"

#define DEBUG_TIMEOUT_MS 1000 * 2

namespace ahri {

/**
 * @brief 协程调度器
 * 
 */
class CoScheduler {
public:
  typedef size_t idx_t;
  typedef std::shared_ptr<CoScheduler> Ptr;

  static CoScheduler *GetSched();

  /**
   * @brief 启动调度
   * 
   * @param n_min_thread 最少使用多少个线程
   * @param n_max_thread 最多使用多少个线程，如果<=n_min_thread在，则使用n_min_thread
   */
  void Start(int n_min_thread, int n_max_thread = 0);

  /**
   * @brief 对于CoScheduler::start函数的封装
   * 
   * @param n_min_thread 
   * @param n_max_thread 
   */
  void Begin(int n_min_thread, int n_max_thread = 0);

  /**
   * @brief 停止工作
   * 
   */
  void Stop();


  void SchedulerTask(std::function<void()> &&fn);

public:
  ~CoScheduler();

  CoScheduler(const CoScheduler &) = delete;

  CoScheduler(const CoScheduler &&) = delete;

  CoScheduler &operator=(const CoScheduler &) = delete;

  CoScheduler &operator=(const CoScheduler &&) = delete;

private:
  CoScheduler();


private:
  /**
   * @brief 创建新的CoExecutor
   * 
   */
  void CreateNewExecutor();

  /**
   * @brief 调度线程的执行函数
   * 
   */
  void DispatcherThreadFunc();

  /**
   * @brief 平等分配任务
   * 
   */
  void DispatchTasksEqually();

  /**
   * @brief 将任务加入一个合适的CoExecutor中
   * 
   * @param tk 任务指针
   */
  void AddTask(const TaskPtr& tk);

private:
  // 锁
  std::mutex m_mtx;
  // 标志启动的锁
  std::mutex m_started_mtx;
  // 保存调度器拥有的执行器指针
  std::vector<CoExecutor::Ptr> m_executors;
  // 调度线程
  Thread m_dispatcher;
  // 最小和最大线程数
  int m_min_thread_cnt = 1;
  int m_max_thread_cnt = 1;
  // 是否停止标记
  bool m_stopping = false;
};

// 简便使用的宏定义
#define g_coscheduler CoScheduler::GetSched()
#define co_sched g_coscheduler

} // namespace src
