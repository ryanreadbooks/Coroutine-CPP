#include <atomic>
#include <iostream>
#include <vector>
#include "coexecutor.h"
#include "thread.h"

using namespace ahri;

CoExecutor::RecoveryEntry entry;
std::atomic<int> gv;

CoExecutor::Ptr executor(new CoExecutor);

void co_func1() {
  std::cout << "Co func1 \n";
  // 主动让出
  CoExecutor::Hold(entry);
  std::cout << "entry.id = " << entry.id << std::endl;
  std::cout << "Co func1 done\n";
}

void co_func2() {
  std::cout << "Co func2 \n";
  // 从恢复点处恢复
  CoExecutor::Wakeup(entry);
  std::cout << "Co func2 done\n";
}

void co_func3() {
  std::cout << "Co func3 \n";
  std::cout << "Co func3 done\n";
}

void coexec_test() {
  std::cout << "Coexec test\n";
  auto co1 =
      std::make_shared<CoExecutor::CoTask>(std::function<void()>(co_func1));
  auto co2 =
      std::make_shared<CoExecutor::CoTask>(std::function<void()>(co_func2));
  auto co3 =
      std::make_shared<CoExecutor::CoTask>(std::function<void()>(co_func3));
  executor->AddTask(co1);
  executor->AddTask(co2);
  executor->AddTask(co3);
}

void test_batch_add_task() {
  auto co1 =
      std::make_shared<CoExecutor::CoTask>(std::function<void()>(co_func1));
  auto co2 =
      std::make_shared<CoExecutor::CoTask>(std::function<void()>(co_func2));
  auto co3 =
      std::make_shared<CoExecutor::CoTask>(std::function<void()>(co_func3));
  std::vector<CoExecutor::CoTaskPtr> tasks{co1, co2, co3};
  executor->AddTask(tasks.begin(), tasks.end());
}

void task1() {
  std::cout << "Global value = " << ++gv << std::endl;
  // if (gv == 5) {
  //   throw std::runtime_error("gv is 5");
  // }
}

void coexec_test_with_thread_add_task() {
  Thread t1([&]() { executor->Process(); }, "ProcessThread");

  Thread t2(
      [&] {
        while (true) {
          sleep(1);
          executor->AddTask(std::function<void()>(task1));
        }
      },
      "AddTaskThread");
  while (true) {
    sleep(2);
    executor->AddTask(std::function<void()>(task1));
  }

  t1.Join();
  t2.Join();
}

int main() {
  coexec_test();
  std::cout
      << "---------------------------------------------------------------\n";
  test_batch_add_task();
  std::cout
      << "---------------------------------------------------------------\n";
  coexec_test_with_thread_add_task();
  return 0;
}