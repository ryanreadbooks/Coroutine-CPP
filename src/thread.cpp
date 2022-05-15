#include <errno.h>
#include <system_error>
#include <cstring>

#include "thread.h"
#include "utils.h"

namespace ahri {

// 线程局部变量
// 线程名字
static thread_local std::string t_thread_name = "UNKNOWN"; // 该名称和t_thread的m_name保持一致
// 当前线程对象
static thread_local Thread* t_thread = nullptr;

Thread::Thread(std::function<void()> cb, const std::string& name) 
  :m_cb(cb), m_name(name) {
    if (name.empty()) {
      m_name = "UNKNOWN";
    }
    t_thread_name = m_name;
    // 同时创建出线程
    int pt = pthread_create(&m_thread, nullptr, &Thread::Run, this);
    if (pt != 0) { // 创建失败
      THROW_SYS_ERROR("pthread_create error");
    }
    m_semaphore.Wait();
}

Thread::~Thread() {
  if (m_thread) {
    pthread_detach(m_thread);
  }
}

void Thread::SetName(const std::string& name) {
  if (name.empty()) {
    return;
  }
  if (t_thread) {
    t_thread->m_name = name;
  }
  t_thread_name = name;
} 

Thread* Thread::GetThis() {
  return t_thread;
}

const std::string& Thread::GetName() {
  return t_thread_name;
}

void Thread::Join() {
  if (m_thread) {
    int ret = pthread_join(m_thread, nullptr);
    if (ret != 0) { // join失败
      THROW_SYS_ERROR("pthread_join error");
    }
    m_joinable = false;
    m_thread = 0;
  }
}

void Thread::Detach() {
  if (m_thread) {
    if (pthread_detach(m_thread) != 0) {
      THROW_SYS_ERROR("pthread_detach error");
    }
    m_joinable = false;
  }
}

void* Thread::Run(void* arg) {
  Thread* self = (Thread*)(arg);
  t_thread = self;
  t_thread_name = t_thread->m_name;
  t_thread->m_id = ahri::GetThreadId();
  // 给当前线程设置名称，名称最多可以接收16个字符
  pthread_setname_np(pthread_self(), t_thread->m_name.substr(0, 15).c_str());

  std::function<void()> cb;
  cb.swap(self->m_cb);
  t_thread->m_semaphore.Post();
  cb(); // 开始运行

  return 0;
}

} // namespace src