#include "ahri/threadpool.h"
#include "ahri/utils.h"
#include <chrono>
#include <iostream>
using namespace std::chrono;

void f1() {
  int i = 0;
  long j = 0;
  printf("Running f1 in thread-%d", ahri::GetThreadId());
  while (++i < 1000000) {
    j += 2;
  }
  std::this_thread::sleep_for(seconds(2));
  printf("Running f1 in thread-%d", ahri::GetThreadId());
}

void f2() {
  int i = 0;
  long j = 0;
  printf("Running f2 in thread-%d", ahri::GetThreadId());
  while (++i < 1000000) {
    j += 3;
  }
  std::this_thread::sleep_for(seconds(10));
  printf("Running f2 in thread-%d", ahri::GetThreadId());
}

void f3() {
  int i = 0;
  long j = 0;
  printf("Running f3 in thread-%d", ahri::GetThreadId());
  while (++i < 1000000) {
    j += 4;
  }
  std::this_thread::sleep_for(seconds(1));
  printf("Running f3 in thread-%d", ahri::GetThreadId());
}

int main() {
//  g_threadpool.Start();
  ahri::ThreadPoolExecutor threadpool(4) ;
  threadpool.Start();
  int t = 0;
  while (threadpool.IsStarted() && ++t < 5) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    threadpool.SchedulerTask(f1);
    threadpool.SchedulerTask(f2);
    threadpool.SchedulerTask(f3);
  }
  printf("%s\n","Gonna stop");
  threadpool.Stop();
  printf("%s\n","Pool stopped");

  return 0;
}