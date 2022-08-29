#include "coroutine.h"
#include "thread.h"
#include "utils.h"


void co_fun1() {
  /* 单独使用协程对象的时候，src::GetCoroutineId()返回-1 */
  std::cout << "In co_fun1(id=" << ahri::GetCoroutineId() << ") step 1...\n";
  ahri::this_coroutine::Yield();
  std::cout << "In co_fun1(id=" << ahri::GetCoroutineId() << ") step 2...\n";
  throw std::runtime_error("Manually throw exception");
  ahri::this_coroutine::Yield();
  std::cout << "In co_fun1(id=" << ahri::GetCoroutineId() << ") step 3... finished\n";
}

void co_fun2() {
  std::cout << "In co_fun2(id=" << ahri::GetCoroutineId() << ") 1...\n";
  ahri::this_coroutine::Yield();
  std::cout << "In co_fun2(id=" << ahri::GetCoroutineId() << ") 3...\n";
  ahri::this_coroutine::Yield();
  std::cout << "In co_fun2(id=" << ahri::GetCoroutineId() << ") 5... finished\n";
}

void co_fun3() {
  std::cout << "In co_fun3(id=" << ahri::GetCoroutineId() << ") 2...\n";
  ahri::this_coroutine::Yield();
  std::cout << "In co_fun3(id=" << ahri::GetCoroutineId() << ") 4...\n";
  ahri::this_coroutine::Yield();
  std::cout << "In co_fun3(id=" << ahri::GetCoroutineId() << ") 6... finished\n";
}

void simple_coroutine_test() {
  ahri::Coroutine::Ptr co1(new ahri::Coroutine(std::function<void()>(co_fun1)));
  co1->Resume();
  std::cout << "After resume-1\n";
  co1->Resume();
  std::cout << "After resume-2\n";
  co1->Resume();
  std::cout << "After resume-3\n";
}

void simple_coroutine_test2() {
  ahri::Coroutine::Ptr co2(new ahri::Coroutine(std::function<void()>(co_fun2)));
  ahri::Coroutine::Ptr co3(new ahri::Coroutine(std::function<void()>(co_fun3)));
  co2->Resume();
  co3->Resume();
  co2->Resume();
  co3->Resume();
  co2->Resume();
  co3->Resume();
}

void coroutine_with_thread() {
  std::vector<ahri::Thread::Ptr> thrs;
  for (int i = 0; i < 5; ++i) {
    thrs.emplace_back(std::make_shared<ahri::Thread>(
                  simple_coroutine_test2, 
                  "thread-" + std::to_string(i)));
  }
  for (size_t i = 0; i < thrs.size(); ++i) {
    thrs[i]->Join();
  }
}

void co_fun4() {
  std::cout << "In co_fun4(id=" << ahri::GetCoroutineId() << ") step 1...\n";
  ahri::this_coroutine::Yield();
  std::cout << "In co_fun4(id=" << ahri::GetCoroutineId() << " finished\n";
}

void co_test3() {
  ahri::Coroutine::Ptr co1(new ahri::Coroutine(co_fun4));
  co1->Resume();
  co1->Resume();
  std::cout << "Status = " << co1->GetStatus() << std::endl;
}

void co_fun1();

// thread_local src::Coroutine main_co;
thread_local ahri::Coroutine::Ptr main_co = ahri::Coroutine::CreateMaster();
ahri::Coroutine co1(co_fun1, 0);

// void co_fun1() {
//   std::cout << "In co_fun1";
//   co1.GiveUp();
//   std::cout << "In co_fun1 done";
// }

int main() {

  co1.Resume();
  std::cout << "Between resume\n";
  co1.Resume();
  std::cout << "DONE\n";

  // simple_coroutine_test();
  // std::cout << "-------------------------------------------------------------------\n";
  // simple_coroutine_test2();
  // std::cout << "-------------------------------------------------------------------\n";
  // coroutine_with_thread();
  // std::cout << "-------------------------------------------------------------------\n";
  // co_test3();

  return 0;
}