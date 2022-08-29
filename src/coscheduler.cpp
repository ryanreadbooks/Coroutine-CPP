#include <map>
#include <thread>

#include "coscheduler.h"

namespace ahri {

CoScheduler *CoScheduler::GetSched() {
  static CoScheduler *sched = new CoScheduler;
  static CoScheduler::Ptr sched_ptr(sched);
  return sched;
}

CoScheduler::CoScheduler() {
  // 至少需要一个执行器
  m_executors.emplace_back(new CoExecutor(0));
}

CoScheduler::~CoScheduler() {
  Stop();
// stop后已经清理了资源，不应该再调其它相关方法
#ifdef DEV_DEBUGGING
  std::cout << "CoScheduler::~CoScheduler in thread-" << ahri::GetThreadId()
            << std::endl;
#endif
}

void CoScheduler::Start(int n_min_thread, int n_max_thread) {
  if (m_stopping) {
    std::cout << "CoScheduler is stopping, can not start" << std::endl;
    return;
  }
  if (!m_started_mtx.try_lock()) {
    throw std::logic_error("Can not repeatedly call CoScheduler::start !");
  }
  if (n_min_thread < 1) {
    n_min_thread = (int)std::thread::hardware_concurrency();
  }
  if (n_max_thread == 0 || n_max_thread < n_min_thread) {
    n_max_thread = n_min_thread;
  }
  m_min_thread_cnt = n_min_thread;
  m_max_thread_cnt = n_max_thread;
  m_executors.reserve(m_max_thread_cnt);
  for (int i = 0; i < m_min_thread_cnt - 1; ++i) {
    CreateNewExecutor();
  }

  if (m_max_thread_cnt > 1) {
    // 启动调度线程
    Thread dispatcher_t(std::bind(&CoScheduler::DispatcherThreadFunc, this),
                        "sched-dispat");
    m_dispatcher.Swap(dispatcher_t);
  } else {
    std::cout << "No dispatcher is needed\n";
  }
  std::cout << "CoScheduler start with m_min_thread_cnt = " << m_min_thread_cnt
            << " m_max_thread_cnt = " << m_max_thread_cnt << std::endl;
  // 每个调度器都有一个主执行器，也就是至少都需要有一个执行器
  CoExecutor::Ptr main_exctr = m_executors.front();
  std::cout << "Executor[0]=>" << m_executors[0]->Id() << " in thread-"
            << GetThreadId() << std::endl;
  for (size_t i = 1; i < m_executors.size(); ++i) {
    std::cout << "Executor[" << i << "]=>" << m_executors[i]->Id()
              << " in thread-" << m_executors[i]->m_process_tid << std::endl;
  }
  // 阻塞当前线程调度
  main_exctr->Process(DEBUG_TIMEOUT_MS);
}

void CoScheduler::Begin(int n_min_thread, int n_max_thread) {
  Thread t([this, n_min_thread,
            n_max_thread]() { this->Start(n_min_thread, n_max_thread); },
           "cosched");
  t.Detach();
}

void CoScheduler::Stop() {
  m_stopping = true;
  for (size_t i = 0; i < m_executors.size(); ++i) {
    m_executors[i]->m_is_stopping = true;  // 退出每一个执行器
  }
  if (!m_executors.empty()) {
    m_executors.clear();
    std::vector<CoExecutor::Ptr>().swap(m_executors);
  }
}

void CoScheduler::SchedulerTask(std::function<void()> &&fn) {
  TaskPtr tk = std::make_shared<Task>(fn);
  AddTask(tk);
}

void CoScheduler::AddTask(const TaskPtr &tk) {
  // 找到一个合适的CoExecutor将任务加进去
  // TODO 现在先随机找一个放进去，改成找一个相对负载低的放进去
  if (m_executors.size() == 1) {
    m_executors[0]->AddTask(tk);
  } else {
    auto id = rand() % m_executors.size();
    //     m_executors[0]->AddTask(tk);
    m_executors[id]->AddTask(tk);
    std::cout << "CoScheduler assign new task to executor-" << id << std::endl;
  }
}

void CoScheduler::CreateNewExecutor() {
  if ((int)m_executors.size() < m_max_thread_cnt) {
    CoExecutor::Ptr co_executor(new CoExecutor(m_executors.size()));
    size_t e_id = m_executors.size();
    // 放在线程中执行executor
    Thread t(
        [=]() {
          std::cout << "CoExecutor-" << e_id << " is now running in thread-"
                    << GetThreadId() << std::endl;
          co_executor->Process(DEBUG_TIMEOUT_MS);
        },
        "executor-" + std::to_string(e_id));
    // 分离
    t.Detach();
    m_executors.push_back(co_executor);
  }
}

void CoScheduler::DispatcherThreadFunc() {
  while (!m_stopping) {
    usleep(100 * 10000);
    std::cout << "CoScheduler::DispatcherThreadFunc\n";
    // TODO 针对一些阻塞的执行器进行处理
    DispatchTasksEqually();
  }
}

void CoScheduler::DispatchTasksEqually() {
  // 计算负载，并且将负载高的执行器的任务分配一些给负载低的执行器
  size_t executor_count = m_executors.size();
  size_t total_runnables = 0;
  size_t total_waitings = 0;
  std::vector<size_t> runnables_counts(executor_count, 0);
  std::vector<size_t> waitings_counts(executor_count, 0);
  for (size_t i = 0; i < executor_count; ++i) {
    runnables_counts[i] = m_executors[i]->GetRunnableCount();
    waitings_counts[i] = m_executors[i]->GetWaitingCount();
    total_runnables += m_executors[i]->GetRunnableCount();
    total_waitings += m_executors[i]->GetWaitingCount();
  }
  size_t total_loads = total_runnables + total_waitings;
  if (total_loads == 0) {
    std::cout << "No dispatching is needed, total loads is 0\n";
    return;
  }
  // 计算所有执行器的平均负载
  size_t avg_load = total_loads / executor_count;
  // 搜集高负载的执行器中取出超过平均负载的部分
  ThreadSafeDeque<TaskPtr> stolen;
  std::map<idx_t, size_t> low_load_executors;  // 低负载的执行器索引和负载大小
  idx_t min_load_idx = 0;  // 记录负载最小的executor所在索引
  size_t min_load = m_executors[0]->GetRunnableCount();
  for (size_t idx = 0; idx < executor_count; ++idx) {
    size_t load = m_executors[idx]->GetRunnableCount();
    std::cout << "Executor-" << m_executors[idx]->Id() << " Load = " << load
              << std::endl;
    if (min_load > load) {
      min_load = load;
      min_load_idx = idx;
    }
    if (load > avg_load) {
      if (m_executors[idx]->IsBlocking()) {
        m_executors[idx]->GiveUpTasks(stolen, 0);
      } else {
        m_executors[idx]->GiveUpTasks(stolen, load - avg_load);
      }
    } else {
      low_load_executors[idx] = load;
    }
  }
  if (stolen.Empty()) {
    std::cout << "No extra tasks collected!!\n";
    return;
  }
  std::cout << "Collected " << stolen.Size() << " tasks from " << executor_count
            << " executors\n";
  // 平均分发所有任务给所有低负载的执行器，先提前满足前面的
  auto first = stolen.begin();
  auto last = first;
  for (auto &item : low_load_executors) {
    idx_t idx = item.first;
    size_t load = item.second;
    size_t n_supply = avg_load - load;  // 需要填补的数量
    if (n_supply == 0) {
      continue;
    }
    last = first + n_supply;
    last = std::min(last, stolen.end());  // 防止越界
    m_executors[idx]->AddTask(first, last);
    std::cout << "CoExecutor-" << m_executors[idx]->Id() << " assigned "
              << (last - first) << " tasks from sched\n";
    first = last;
    if (first >= stolen.end()) {
      // 提前分配完了
      break;
    }
  }
  if (first < stolen.end()) {  // 检查是否有剩余
                               // 给到一开始负载最小的executor
    m_executors[min_load_idx]->AddTask(first, stolen.end());
    std::cout << "Executor-" << m_executors[min_load_idx]->Id()
              << " got assigned the rest\n";
  }
}

}  // namespace src