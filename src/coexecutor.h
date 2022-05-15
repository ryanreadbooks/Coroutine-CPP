#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <vector>

#include "containers.hpp"
#include "coroutine.h"

#define TRIGGER_GC_TASK_SIZE 64
#define COROUTINE_TIMEDOUT_MS 100
#define COROUTINE_TIMEDOUT_US COROUTINE_TIMEDOUT_MS * 1000
#define GC_INTERVAL_MS 2000

namespace ahri {
class Coroutine;

/**
 * @brief 一个线程包含一个CoExecutor用来真正执行协程队列中的协程
 *
 */
class CoExecutor {
  friend class CoScheduler;

public:
  using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

  /**
   * @brief 表示一个任务
   *
   */
  struct CoTask {
    // 任务协程
    std::shared_ptr<Coroutine> co;
    // 处理器指针，该任务属于哪个处理器处理
    CoExecutor *proc;

    CoTask(std::shared_ptr<Coroutine> &c) : co(c) {}

    CoTask(std::function<void()> &&f) : co(std::make_shared<Coroutine>(std::move(f), 0)) {}

    CoTask(std::function<void()> &f) : co(std::make_shared<Coroutine>(f, 0)) {}
  };

  using CoTaskWeakPtr = std::weak_ptr<CoTask>;
  using CoTaskPtr = std::shared_ptr<CoTask>;

  /**
   * @brief 挂起后的恢复入口
   *
   */
  struct RecoveryEntry {
    // 任务的弱引用
    CoTaskWeakPtr tk;
    // executor所属id
    int32_t id;

    explicit operator bool() const { return tk.lock() != nullptr && id != 0; }

    friend bool operator==(const RecoveryEntry &one, const RecoveryEntry &oth) {
      return one.tk.lock() == oth.tk.lock() && one.id == oth.id;
    }

    friend bool operator<(const RecoveryEntry &one, const RecoveryEntry &oth) {
      return one.tk.lock() < oth.tk.lock() && one.id < oth.id;
    }

    inline bool Expired() const {
      if (!tk.lock()) {
        return false;
      }
      return true;
    }
  };

public:
  typedef std::shared_ptr<CoExecutor> Ptr;

  inline int32_t Id() const { return m_id; }

  inline uint64_t GetSwitchCount() const { return m_switch_cnt; }

  inline size_t GetRunnableCount() const { return m_runnable_queue.Size(); }

  inline size_t GetWaitingCount() const { return m_waiting_queue.Size(); }

  inline size_t GetValidTasksCount() const { return m_runnable_queue.Size() + m_waiting_queue.Size(); }

  inline uint64_t GetStartElapse() const { return m_start_elapse; }

  inline uint64_t GetCurrentElapse() const { return GetCurrentMs() - m_start_elapse; }

  /**
   * @brief 获取执行的效率值（定义为：每毫秒执行的协程数）
   * 
   * @return double 
   */
  inline double GetExecEfficiency() const { return (double) m_switched_cnt / GetCurrentElapse(); }

  /**
   * @brief 获取当前线程的CoExecutor指针
   *
   * @return CoExecutor*&
   */
  static CoExecutor *&GetCurrentExecutor() {
    static thread_local CoExecutor *co_exec = nullptr;
    return co_exec;
  }

  /**
   * @brief 获取当前正在执行的协程
   *
   * @return CoTaskPtr
   */
  static CoTaskPtr GetCurrentTask();

  /**
   * @brief 挂起当前执行的协程
   *
   */
  static void YieldCurrent();

  /**
   * Get master coroutine in current executor
   * @return
   */
  static Coroutine *GetMasterCo();

  /**
   * @brief 挂起当前的协程
   *
   * @param out 返回参数，重新唤醒的入口
   */
  static void Hold(CoExecutor::RecoveryEntry &out);

  /**
   * @brief 挂起当前协程, 并在指定时间后自动唤醒
   * 
   * @param dur 挂起时间，毫秒
   */
  static void HoldFor(const std::chrono::microseconds &dur);

  /**
   * Hold current coroutine until certain timepoint
   * @param tp
   */
  static void HoldUntil(const TimePoint &tp);

  /**
   * @brief 唤醒协程
   *
   * @param entry 唤醒协程的入口
   * @return true
   * @return false
   */
  static bool Wakeup(const RecoveryEntry &entry);

  /**
   * @brief 唤醒所有在等待队列中的任务
   * 
   */
  static void WakeupAll();

  /**
   * @brief 是否停止工作，停止之后不能重新启动
   *
   * @return true
   * @return false
   */
  inline bool IsStopped() const { return m_is_stopping; }

  /**
   * @brief 判断是否阻塞在某个协程上（某个协程执行耗时太久）
   * 如果当前的协程执行的时间超过设定的超时时间，
   * 并且调度的次数和完成调度的次数不相同 则视作阻塞住了
   *
   * @return true 阻塞了
   * @return false 没有阻塞
   */
  inline bool IsBlocking() const { return GetCurrentUs() - m_tick > COROUTINE_TIMEDOUT_US && m_switch_cnt != m_switched_cnt; }

  /**
   * @brief 添加单个任务
   *
   * @param tk
   */
  void AddTask(CoTaskPtr tk);

  /**
   * @brief 以函数的形式添加任务
   *
   */
  void AddTask(std::function<void()> &&fn);

  /**
   * @brief 批量添加任务
   *
   * @tparam Iterator
   * @param begin
   * @param end
   */
  template<typename Iterator>
  void AddTask(Iterator begin, Iterator end) {
    std::unique_lock<std::mutex> lk(m_runnable_queue.LockRef());
    size_t count = 0;
    while (begin != end) {
      m_runnable_queue.PushBackUnsafe(*begin);
      ++begin;
      ++count;
    }
    if (m_waiting) {
      m_cv.notify_all();
    }
    std::cout << count << " task(s) added for executor-" << m_id << std::endl;
  }

public:
  /**
   * @brief 执行调度
   * 
   * @param timeout_seconds 持续没有任务执行退出等待时间，为0表示一直等待。单位ms
   */
  void Process(uint64_t timeout_miliseconds = 0);

  bool WakeupFromEntry(const RecoveryEntry &entry);

  void WakeupAllTasks();

public:
  explicit CoExecutor(int32_t id = 0);

  ~CoExecutor();

  CoExecutor(const CoExecutor &) = delete;

  CoExecutor(const CoExecutor &&) = delete;

  CoExecutor &operator=(const CoExecutor &) = delete;

  CoExecutor &operator=(const CoExecutor &&) = delete;

private:
  /**
   * @brief 条件变量等待，等待有任务可以处理
   *
   */
  void WaitForCondition();

  /**
   * @brief 条件变量等待一定时间，超时退出
   *
   * @param seconds 等待的秒数
   */
  void WaitForConditionFor(uint64_t miliseconds);

  /**
   * @brief 在条件变量上唤醒，通知处理
   *
   */
  void NotifyCondition();

  /**
   * @brief 清除完成任务队列中的任务
   *
   */
  void Clean();

  /**
  * @brief 挂起指定任务，并且返回任务的恢复入口
  *
  * @param tk 需要挂起的任务
  * @param out 重新唤醒任务的入口
  */
  void HoldThere(CoTaskPtr tk, CoExecutor::RecoveryEntry &out);

  /**
   * @brief 主动放弃一些没来得及处理的任务
   * 
   * @param giveups 放弃任务的数量，为0表示放弃当前所有未处理的任务
   * @param n 返回结果
   */
  void GiveUpTasks(ThreadSafeDeque<CoTaskPtr> &giveups, size_t n = 0);

  /**
   * @brief 条件变量的等待判断条件
   * 
   * @return true 
   * @return false 
   */
  bool Predicate() const;

  /**
   * @brief 指定下一个要运行的任务
   * 
   * @param from_awoken 从哪里分配
   * @return true 从awoken队列中分配了
   * @return false 从runnable队列中分配了
   */
  bool AssignRunnableTask(bool from_awoken);

public:
  static void CoYield();

private:
  // id
  int32_t m_id;
  // 运行所在的线程id
  int32_t m_process_tid = -1;
  // 可以运行的协程队列
  ThreadSafeDeque<CoTaskPtr> m_runnable_queue;
  // 正在hold状态的协程任务
  ThreadSafeDeque<CoTaskPtr> m_waiting_queue;
  // 运行完成的协程任务队列
  ThreadSafeDeque<CoTaskPtr> m_finished_queue;
  // hold了之后的协程的等待队列
  ThreadSafeDeque<CoTaskPtr> m_awoken_queue;
  // 当前正在运行的协程
  CoTaskPtr m_running_task = nullptr;
  // 条件变量
  std::condition_variable m_cv;
  // 锁
  mutable std::mutex m_mtx;
  // 是否正在等待有任务可以处理
  std::atomic_bool m_waiting;
  // 是否将要停止
  std::atomic_bool m_is_stopping{false};
  // 协程调度次数
  volatile uint64_t m_switch_cnt = 0;
  // 调度完成的次数
  volatile uint64_t m_switched_cnt = 0;
  // 是否活跃的标记
  bool m_active = false;
  // 当前执行的协程开始的时间戳(单位us)
  volatile uint64_t m_tick = 0;
  // 执行器开始执行的开始时间
  uint64_t m_start_elapse = GetCurrentMs();
  // 上次回收垃圾的时间戳
  uint64_t m_last_gc_tick = 0;
  // 马上gc
  bool m_clean_right_now = false;
};

typedef CoExecutor::CoTaskPtr TaskPtr;
typedef CoExecutor::CoTask Task;

namespace this_coroutine {

void Yield();

int32_t GetId();

} // namespace this_coroutine;

} // namespace src