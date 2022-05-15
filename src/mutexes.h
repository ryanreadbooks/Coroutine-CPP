#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <cstdint>

#include "nocopyable.h"


namespace ahri {

/**
 * @brief linux信号量封装
 * 
 */
class Semaphore {
public:
  /**
   * @brief 构造函数
   * 
   * @param count 初始计数器
   */
  Semaphore(uint32_t count = 0);

  ~Semaphore();

  /**
   * @brief 等待信号量
   * 
   */
  void Wait();

  /**
   * @brief post信号量
   * 
   */
  void Post();

private:
  sem_t m_semaphore;
};

/**
 * @brief linux下读写锁的封装
 * 
 */
class RWMutex : public NoCopyable {
public:
  RWMutex() {
    m_mutex = PTHREAD_RWLOCK_INITIALIZER;
  }

  ~RWMutex() {
    pthread_rwlock_destroy(&m_mutex);
  }

  /**
   * @brief 上读锁
   * 
   */
  void RdLock() {
    pthread_rwlock_rdlock(&m_mutex);
  }

  /**
   * @brief 上写锁
   * 
   */
  void WrLock() {
    pthread_rwlock_wrlock(&m_mutex);
  }

  /**
   * @brief 解锁
   * 
   */
  void Unlock() {
    pthread_rwlock_unlock(&m_mutex);
  }
  
private:
  pthread_rwlock_t m_mutex;
};

/**
 * @brief 读锁的LockGuard
 * 
 */
class RdLockGuard : public NoCopyable {
public:
  RdLockGuard(RWMutex& mutex) : m_mutex(mutex) {
    // 上锁
    m_mutex.RdLock();
    m_locked = true;
  } 

  ~RdLockGuard() {
    // 解锁
    if (m_locked) {
      m_mutex.Unlock();
      m_locked = false;
    }
  }

private:
  RWMutex& m_mutex;
  bool m_locked = false;
};

/**
 * @brief 写锁的LockGuard
 * 
 */
class WrLockGuard : public NoCopyable {
public:
  WrLockGuard(RWMutex& mutex) : m_mutex(mutex) {
    // 上锁
    m_mutex.WrLock();
    m_locked = true;
  } 

  ~WrLockGuard() {
    // 解锁
    if (m_locked) {
      m_mutex.Unlock();
      m_locked = false;
    }
  }

private:
  RWMutex& m_mutex;
  bool m_locked = false;
};

} // namespace src