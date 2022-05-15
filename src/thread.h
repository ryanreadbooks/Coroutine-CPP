#pragma once

#include <pthread.h>
#include <memory>
#include <string>
#include <functional>

#include "nocopyable.h"
#include "mutexes.h"

namespace ahri {

/**
 * @brief 线程库的封装
 * 
 */
class Thread {
public:
  typedef std::shared_ptr<Thread> Ptr;

  Thread() {}

  /**
   * @brief 构造函数
   * 
   * @param cb 线程执行函数
   * @param name 线程名称
   */
  Thread(std::function<void()> cb, const std::string& name = "");

  ~Thread();

  /**
   * @brief 获取线程id
   * 
   * @return pid_t 线程id
   */
  pid_t GetTid() const { return m_id; }

  /**
   * @brief 获取线程名称
   * 
   * @return const std::string& 线程名称
   */
  const std::string& GetTName() const { return m_name; }

  /**
   * @brief 设置线程名称
   * 
   * @param name 线程名称
   */
  static void SetName(const std::string& name);

  /**
   * @brief 获取线程对象指针
   * 
   * @return Thread* 线程对象指针
   */
  static Thread* GetThis();

  /**
   * @brief Get the name object获取线程名称
   * 静态方法，通过Thread::GetName()调用
   * 
   * @return const std::string& 线程名称
   */
  static const std::string& GetName();

  /**
   * @brief 等待线程结束
   * 
   */
  void Join();

  /**
   * @brief 分离线程
   * 
   */
  void Detach();

  bool Joinable() const { return m_joinable; }

  void Swap(Thread& x) {
    std::swap(m_thread, x.m_thread);
    std::swap(m_name, x.m_name);
    std::swap(m_id, x.m_id);
    std::swap(m_joinable, x.m_joinable);
    std::swap(m_semaphore, x.m_semaphore);
  }

private:
  /**
   * @brief 线程在内部真正执行的函数
   * 
   * @param arg 参数
   * @return void* 返回值
   */
  static void* Run(void* arg);

private:
  // 函数体
  std::function<void()> m_cb;
  // 线程名称
  std::string m_name;
  // 线程id
  pid_t m_id = -1;
  // 线程体
  pthread_t m_thread = 0;
  // 信号量
  Semaphore m_semaphore;
  // 是否能够join
  bool m_joinable = true;
};

} // namespace src