#include "threadpool.h"
namespace ahri {

ThreadPoolExecutor::ThreadPoolExecutor(size_t n_threads)
    : m_thread_count(n_threads),
      m_stopped(true), m_is_stopping(false), m_started(false) {
  m_thread_count = m_thread_count == 0 ? std::thread::hardware_concurrency() : m_thread_count;
}

ThreadPoolExecutor::~ThreadPoolExecutor() {
  if (m_is_stopping) {
    m_stopped = true;
    m_is_stopping = false;
    m_started = false;
    if (!m_tasks_queue.Empty()) {
      m_tasks_queue.Clear();
    }
    if (!m_threads.empty()) {
      m_threads.clear();
    }
  }
}

void ThreadPoolExecutor::SchedulerTask(const TaskF& task) {
  if (!m_stopped && !m_is_stopping) {
    m_tasks_queue.PushBack(task);
    NotifyCondition();
  }
}

void ThreadPoolExecutor::Start() {
  if (!m_started && m_stopped) {
    m_threads.reserve(m_thread_count);
    m_started = true;
    m_stopped = false;
    for (size_t i = 0; i < m_thread_count; ++i) {
      m_threads.emplace_back(std::thread(&ThreadPoolExecutor::Runnable, this));
    }
  }
}

void ThreadPoolExecutor::Stop(bool join) {
  if (!m_stopped && m_started) {
    m_is_stopping = true;
    NotifyCondition();  // 唤醒所有的任务让他们结束
    if (join) {
      WaitForTaskDone();  // 等待所有执行的任务结束
    } else {
      DetachAllThreads();
    }
  }
}

void ThreadPoolExecutor::Runnable() {
  // 从任务队列中取出任务执行
  while (m_started && !m_is_stopping) {
    WaitForCondition();
    // 出来就可以去取任务
    if (!m_tasks_queue.Empty()) {
      UniLock lk(m_tasks_queue.LockRef());
      auto fn = m_tasks_queue.FrontNoLock();
      m_tasks_queue.PopFrontUnsafe();
      lk.unlock();
      fn();
      TaskF().swap(fn);
    }
  }
}

void ThreadPoolExecutor::WaitForCondition() {
  // 等待任务队列不为空
  UniLock lk(m_mtx);
  m_cv.wait(lk, [this]() {
    return !m_tasks_queue.Empty() || m_is_stopping;
  });
}

void ThreadPoolExecutor::DetachAllThreads() {
  for (auto &&t : m_threads) {
    t.detach();
  }
}

void ThreadPoolExecutor::WaitForTaskDone() {
  for (auto &&t: m_threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

void ThreadPoolExecutor::NotifyCondition() {
  if (!m_tasks_queue.Empty()) {
    UniLock lk(m_mtx);
    m_cv.notify_all();
  }
}

} // namespace src