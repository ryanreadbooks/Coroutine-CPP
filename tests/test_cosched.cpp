#include "ahri/coscheduler.h"

#include <thread>
#include <chrono>
using std::chrono::milliseconds;

using namespace ahri;

void func1() {
  std::cout << "FUNC1 in thread-" << GetThreadId() << ", co-" << ahri::GetCoroutineId() << std::endl;
  co_sleep(100 * 5);
  long t = 0;
//  std::this_thread::sleep_for(milliseconds(100));
  for (int i = 1; i < 1000000; ++i)
  {
    ++t;
  }
  std::cout << "FUNC1 Done thread-" << GetThreadId() << ", co-" << ahri::GetCoroutineId() << std::endl;
}

void func2() {
  std::cout << "FUNC2 in thread-" << GetThreadId() << ", co-" << ahri::GetCoroutineId() << std::endl;
  long t = 0;
  co_sleep(50);
//  std::this_thread::sleep_for(milliseconds(50));

  for (long i = 1; i < 9040000; ++i) {
    ++t;
  }
  std::cout << "FUNC2 Done thread-" << GetThreadId() << ", co-" << ahri::GetCoroutineId() << std::endl;
}

void func3() {
  std::cout << "FUNC3 in thread-" << GetThreadId() << ", co-" << ahri::GetCoroutineId() << std::endl;
  long t = 0;
  co_sleep(300);
//  std::this_thread::sleep_for(milliseconds(300));

  for (long i = 1; i < 5515000; ++i) {
    ++t;
  }
  std::cout << "FUNC3 Done thread-" << GetThreadId() << ", co-" << ahri::GetCoroutineId() << std::endl;
}

// 基础功能测试
void primary_cosched_test() {
  // auto sched = CoScheduler::GetSched();
  co_sched->SchedulerTask(std::function<void()>(func1));
  co_sched->SchedulerTask(std::function<void()>(func2));

  co_sched->Start(2);
}

// 测试调度功能
void test_coshed_dispatcher() {
  int task_required = 50;
  int cur_task = 0;
  Thread t([&]()
           {
    // 生成1000个任务
    while (++cur_task < task_required) {
      usleep(50 * 1000);
      int option = rand() % 3;
      if (option == 0) {
        co_sched->SchedulerTask(std::function<void()>(func1));
      } else if (option == 1) {
        co_sched->SchedulerTask(std::function<void()>(func2));
      } else {
        co_sched->SchedulerTask(std::function<void()>(func3));
      }
    }
    std::cout << "added task finished, exit\n"; }
  );
  co_sched->Start(3);
  t.Join();
}

int main() {
  // primary_cosched_test();
  test_coshed_dispatcher();
  return 0;
}