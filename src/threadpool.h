#pragma once

#include <thread>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

#include "singleton.hpp"
#include "containers.hpp"

namespace ahri {

class ThreadPoolExecutor {
  using TaskF = std::function<void()>;
  using TaskQueue = ThreadSafeDeque<TaskF>;
  using UniLock = std::unique_lock<std::mutex>;
  using LockGuard = std::lock_guard<std::mutex>;
public:
  explicit ThreadPoolExecutor(size_t n_threads = 0);

  ~ThreadPoolExecutor();

  void SchedulerTask(const TaskF& task);

  void Start();

  void Stop(bool join = false);

  bool IsStarted() const { return m_started; }

  bool IsStopped() const { return m_stopped; }

private:

  /**
   * 内部线程运行的函数
   */
  void Runnable();

  /**
   * Wait for the task queue not empty
   */
  void WaitForCondition();

  void WaitForTaskDone();

  void DetachAllThreads();

  /**
   * notify that task queue is not empty
   */
  void NotifyCondition();

private:
  // 线程数量
  size_t m_thread_count;
  // 线程池中的所有线程
  std::vector<std::thread> m_threads;
  // 锁
  std::mutex m_mtx;
  // 条件变脸，检查任务队列是否为空
  std::condition_variable m_cv;
  // 任务队列
  TaskQueue m_tasks_queue;
  std::atomic_bool m_stopped;
  std::atomic_bool m_is_stopping;
  // 开始标记
  std::atomic_bool m_started;
};

typedef Singleton<ThreadPoolExecutor> ThreadPoolExecutorInstance;
typedef ThreadPoolExecutorInstance ThreadPool;

#define g_threadpool ahri::ThreadPool::GetInstance()

} // namespace src