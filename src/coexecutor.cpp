#include <algorithm>

#include "coexecutor.h"
#include "utils.h"

namespace ahri {

// 线程局部变量，每个线程都有一个主协程和正在执行的协程
// 主协程作为协程间切换的中介
static thread_local Coroutine::Ptr st_master_co{new Coroutine};
// 当前正在执行的协程
static thread_local Coroutine::Ptr st_cur_co = nullptr;

CoExecutor::CoExecutor(int32_t id) : m_id(id), m_waiting(true) {}

CoExecutor::~CoExecutor() {
  if (!m_finished_queue.Empty()) {
    Clean();
  }
  ThreadSafeDeque<CoTaskPtr>().Swap(m_runnable_queue);
  ThreadSafeDeque<CoTaskPtr>().Swap(m_waiting_queue);
}

TaskPtr CoExecutor::GetCurrentTask() {
  return GetCurrentExecutor() == nullptr
         ? nullptr
         : GetCurrentExecutor()->m_running_task;
}

Coroutine *CoExecutor::GetMasterCo() {
  return st_master_co.get();
}

void CoExecutor::Hold(CoExecutor::RecoveryEntry &out) {
  auto cur_executor = GetCurrentTask() ? GetCurrentTask()->proc : nullptr;
  if (!cur_executor) {
    return;
  }
  cur_executor->HoldThere(cur_executor->m_running_task, out);
}

void CoExecutor::HoldFor(const std::chrono::microseconds &dur) {

}

void CoExecutor::HoldUntil(const TimePoint &tp) {

}

bool CoExecutor::Wakeup(const CoExecutor::RecoveryEntry &entry) {
  if (!entry) {
    return false;
  }
  auto cur_executor = entry.tk.lock()->proc;
  if (!cur_executor) {
    return false;
  }
  return cur_executor->WakeupFromEntry(entry);
}

void CoExecutor::WakeupAll() {
  auto cur_executor = GetCurrentExecutor();
  if (!cur_executor) {
    return;
  }
  cur_executor->WakeupAllTasks();
}

bool CoExecutor::AssignRunnableTask(bool from_awoken) {
  if (from_awoken) { // 从awoken队列分配
    m_running_task = m_awoken_queue.Front();
    m_awoken_queue.PopFront();
    return true;
  } else { // 从runnable队列分配
    m_running_task = m_runnable_queue.Front();
    m_runnable_queue.PopFront();
    return false;
  }
}

void CoExecutor::Process(uint64_t timeout_miliseconds) {
  std::cout << "CoExecutor::Process is running in thread-" << GetThreadId() << std::endl;;
  m_process_tid = GetThreadId();
  GetCurrentExecutor() = this;
  bool last_retrieve_from_awoken = false;
  while (!IsStopped()) {
    // 取任务，如果没有任务，则在条件变量上等待
    {
      std::unique_lock<std::mutex> lk(m_mtx);
      // 在runnable_queue和m_awoken_queue上交替去任务
      if (!m_runnable_queue.Empty() &&
          !m_awoken_queue.Empty()) { // 两个队列都不为空，交替去任务
        if (last_retrieve_from_awoken) {
          last_retrieve_from_awoken = AssignRunnableTask(false);
        } else {
          last_retrieve_from_awoken = AssignRunnableTask(true);
        }
      } else if (m_runnable_queue.Empty() &&
                 !m_awoken_queue.Empty()) { // r为空，a不为空
        last_retrieve_from_awoken = AssignRunnableTask(true);
      } else if (!m_runnable_queue.Empty() &&
                 m_awoken_queue.Empty()) { // r不为空，a为空
        last_retrieve_from_awoken = AssignRunnableTask(false);
      } else { // 两个队列都为空
        lk.unlock();
        if (timeout_miliseconds == 0) {
          WaitForCondition(); // 等待任务加入
        } else {
          WaitForConditionFor(timeout_miliseconds);
        }
        if (m_clean_right_now && !m_finished_queue.Empty()) {
          m_clean_right_now = false;
          Clean();
        }
        continue;
      }
      m_running_task->proc = this;
      st_cur_co = m_running_task->co;
      // 将任务协程换入，返回之后表示被换出或者执行完成了
      // std::cout << "Co-" << m_running_task->co->get_id()
      //                       << " got resumed, runnableQueue size is "
      //                       << m_runnable_queue.size()
      //                       << " waitingQueue size is " << m_waiting_queue.size() << std::endl;
      ++m_switch_cnt;
      m_tick = GetCurrentUs();
      m_running_task->co->Resume();
      ++m_switched_cnt;
      // std::cout << "Now waitingQueue size is " << m_waiting_queue.size() << std::endl;
      // 返回后判断任务的状态，对应有不同的操作
      switch (m_running_task->co->GetStatus()) {
        case Coroutine::Status::HOLD:
          // std::cout << "co-" << m_running_task->co->get_id()
          //                       << " m_running_task is HOLD" << std::endl;
          break;
        case Coroutine::Status::FINISHED:
          // std::cout << "co-" << m_running_task->co->get_id()
          //                       << " m_running_task is FINISHED" << std::endl;
          // 任务完成后放入完成任务队列中
          m_finished_queue.PushBack(m_running_task);
          m_running_task = nullptr;
          break;
        case Coroutine::Status::EXCEPT:
          // std::cout << "co-" << m_running_task->co->get_id()
          //                       << " m_running_task is EXCEPT" << std::endl;
          m_finished_queue.PushBack(m_running_task);
          if (m_running_task->co->GetException()) {
            std::exception_ptr ex_ptr =
                m_running_task->co->GetException(); // 复制一份，防止对象销毁
            m_running_task = nullptr;
            std::rethrow_exception(ex_ptr); // 重新抛出协程中出现的异常
          }
          break;
        default:
          break;
      }
      st_cur_co.reset();
      if (m_finished_queue.Size() >= TRIGGER_GC_TASK_SIZE) {
        Clean();
      }
    }
  }
}

void CoExecutor::HoldThere(CoTaskPtr tk, CoExecutor::RecoveryEntry &out) {
  // std::cout << "Try to hold co-" << m_running_task->co->get_id()
  //                       << " in thread-" << get_thread_id();
  AHRI_ASSERT(tk == m_running_task);
  AHRI_ASSERT(tk->co->GetStatus() == Coroutine::Status::RUNNING);
  // 获取下一个任务，将当前任务移除
  m_waiting_queue.PushBack(m_running_task);
  out = RecoveryEntry{CoTaskWeakPtr(tk), m_id};
  m_running_task->co->GiveUp();
}

bool CoExecutor::WakeupFromEntry(const CoExecutor::RecoveryEntry &entry) {
  // std::cout << "CoExecutor::WakeupFromEntry entry is not null, recovery is allowed";
  // 将任务重新放回到m_awoken_queue中，将其从waiting中移除
  auto it = std::find(m_waiting_queue.begin(), m_waiting_queue.end(), entry.tk.lock());
  if (it == m_waiting_queue.end()) {
    return false;
  }
  m_waiting_queue.Erase(it);
  m_awoken_queue.PushBack(entry.tk.lock());  // 放入被唤醒的任务队列
  return true;
}

void CoExecutor::WakeupAllTasks() {
  // 全部放回到runnable中排队
  while (!m_waiting_queue.Empty()) {
    m_awoken_queue.PushBack(m_waiting_queue.Front());
    m_waiting_queue.PopFront();
  }
}

void CoExecutor::AddTask(CoTaskPtr tk) {
  if (!tk) {
    return;
  }
  m_runnable_queue.PushBack(tk);
  if (m_waiting) {
    m_cv.notify_all();
    // std::cout << "Notified!!" << std::endl;
  }
  std::cout << "Task added for executor-" << m_id << std::endl;
}

void CoExecutor::AddTask(std::function<void()> &&fn) {
  AddTask(std::make_shared<CoTask>(std::move(fn)));
}

void CoExecutor::WaitForCondition() {
  std::unique_lock<std::mutex> lk(m_mtx);
  // 等待runnable队列有任务可以去处理
  m_waiting = true;
  std::cout << "CoExecutor-" << m_id << " waiting for condition" << std::endl;

  m_cv.wait(lk, [this]() { return this->Predicate(); });
  // 等待后，占有锁
  m_waiting = false;
  if ((GetCurrentMs() - this->m_last_gc_tick) > GC_INTERVAL_MS) {
    m_clean_right_now = true;
  }
}

void CoExecutor::WaitForConditionFor(uint64_t miliseconds) {
  std::unique_lock<std::mutex> lk(m_mtx);
  std::cout << "Waiting for condition for " << miliseconds
            << " millisecond(s)" << std::endl;
  m_waiting = true;
  if (!m_cv.wait_for(lk, std::chrono::milliseconds(miliseconds),
                     [this]() { return this->Predicate(); })) {
    // 超时仍未就绪
    std::cout << "Waiting for condition variable timedout ("
              << miliseconds << " millisecond(s))" << std::endl;
    m_is_stopping = true;
  }

  if ((GetCurrentMs() - this->m_last_gc_tick) > GC_INTERVAL_MS) {
    m_clean_right_now = true;
  }

  m_waiting = false;
}

void CoExecutor::NotifyCondition() {
  std::unique_lock<std::mutex> lk(m_mtx);
  if (!m_runnable_queue.Empty() &&
      m_waiting) { // 任务队列不为空，并且当前在等待状态，则通知
    m_cv.notify_all();
  }
}

void CoExecutor::Clean() {
  m_last_gc_tick = GetCurrentMs();
  std::cout << "Trigger executor-" << m_id << " cleaning in thread-"
            << GetThreadId() << std::endl;
  ThreadSafeDeque<CoTaskPtr>().Swap(m_finished_queue);
}

void CoExecutor::YieldCurrent() {
  auto tk = GetCurrentTask();
  AHRI_ASSERT(tk != nullptr);
  tk->co->GiveUp();
}

void CoExecutor::GiveUpTasks(ThreadSafeDeque<CoTaskPtr> &giveups, size_t n) {
  std::cout << "CoExecutor-" << m_id << " is ready to give up "
            << (n == 0 ? m_runnable_queue.Size() : n) << " tasks" << std::endl;
  std::lock_guard<std::mutex> lk(m_runnable_queue.LockRef());
  if (n > 0) { // give up some
    m_runnable_queue.PopBackAndAppendUnsafe(n, giveups);
  } else { // give up all
    m_runnable_queue.PopBackAndAppendUnsafe(m_runnable_queue.SizeNoLock(), giveups);
  }
}

bool CoExecutor::Predicate() const {
  // 退出条件变量的条件
  // 1、可执行任务队列不为空
  // 2、准备停止作业
  // 3、当前的运行队列为空，并且完成任务队列不为空，并且上一次GC的时间已经超过设定阈值
  // 4、有队列被唤醒并且加入了awoken_queue中
  bool has_task = !this->m_runnable_queue.Empty();
  bool has_done_task = !this->m_finished_queue.Empty();
  bool gonna_stop = this->m_is_stopping;
  bool timeout = (GetCurrentMs() - this->m_last_gc_tick) > GC_INTERVAL_MS;
  bool need_gc = !has_task
                 && this->m_last_gc_tick != 0
                 && has_done_task
                 && timeout;
  bool has_task_awoken = !m_awoken_queue.Empty();
  return has_task || gonna_stop || need_gc || has_task_awoken;
}

void CoExecutor::CoYield() {
  auto proc = CoExecutor::GetCurrentExecutor();
  if (proc) {
    proc->YieldCurrent();
  }
}

namespace this_coroutine {

void Yield() {
  CoExecutor::CoYield();
}

int32_t GetId() {
  TaskPtr tk = CoExecutor::GetCurrentTask();
  if (tk && tk.use_count() != 0 && tk->co) {
    return tk->co->GetId();
  }
  return -1;
}

} // namespace this_coroutine

} // namespace src
