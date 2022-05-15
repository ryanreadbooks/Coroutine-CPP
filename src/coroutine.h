#pragma once

#include <ucontext.h>
#include <memory>
#include <functional>
#include <atomic>

#include "nocopyable.h"
#include "utils.h"
#include "coexecutor.h"

#define DEFAULT_STACK_SIZE 1024 * 128

#define co_sleep(milli) do { std::this_thread::sleep_for(milliseconds(milli)); } while (0)

namespace ahri {
class CoExecutor;

/**
 * @brief 协程全局信息
 *
 */
namespace coroutine_meta {

static size_t GetCoCount();

}; // CoroutineMeta

class Coroutine : public std::enable_shared_from_this<Coroutine> {
  friend class CoExecutor;

public:
  typedef std::shared_ptr<Coroutine> Ptr;
  typedef std::function<void()> Executable;

  /**
   * @brief 写成的五个状态
   *
   */
  enum Status {
    IDLE,     // 空闲，默认状态
    RUNNING,  // 正在运行
    FINISHED, // 运行正常结束
    HOLD,     // 挂起
    EXCEPT    // 运行过程中出现异常退出
  };

public:
  /**
   * @brief 默认构造函数提供一个空协程，不分配栈空间，也不指定执行函数
   * 
   */
  Coroutine();

  /**
   * @brief 返回用默认构造函数构造的Coroutine智能指针
   * 
   * @return Coroutine::Ptr 
   */
  static Coroutine::Ptr CreateMaster();

  /**
   * @brief 对外接口
   *
   */
public:
  /**
   * @brief 构造一个普通的Coroutine对象
   * 
   * @param executable 需要执行的函数
   * @param ss 分配的栈大小，传入0表示使用默认值
   */
  Coroutine(const Executable &executable, size_t ss = 0);

  ~Coroutine();

  size_t GetStackSize() const { return m_stacksize; }

  ucontext_t &GetContext() { return m_ctx; }

  int64_t GetId() const { return m_id; }

  Status GetStatus() const { return m_status; }

  std::string GetStatusAsString() const;

  void SetStatus(const Status st) { m_status = st; }

  std::exception_ptr GetException() const { return m_ex_ptr; }

  uint64_t GetYieldCount() const { return m_yield_cnt; }

  /**
   * @brief 将协程对象换入成当前协程并执行
   * 
   */
  void Resume();

  /**
   * @brief 当前协程让出
   * 
   */
  void GiveUp();

private:
  inline bool IsIdle() const { return m_status == IDLE; }

  inline bool IsRunning() const { return m_status == RUNNING; }

  inline bool IsHold() const { return m_status == HOLD; }

  inline bool IsFinished() const { return m_status == FINISHED; }

  inline bool IsExcept() const { return m_status == EXCEPT; }

  inline bool CanDestroy() const { return IsIdle() || IsFinished() || IsExcept(); }

  inline bool CanResume() const { return IsIdle() || IsHold(); }

  void Run();

  static void StaticRun(uint32_t low32, uint32_t high32);

private:
  // 栈大小
  size_t m_stacksize;
  // 栈地址
  void *m_stack;
  // ucontext上下文
  ucontext_t m_ctx;
  // id
  int64_t m_id = -1;
  // 执行的函数
  Executable m_func;
  // 状态
  Status m_status = IDLE;
  // 如果发生异常后，保存异常对象指针
  std::exception_ptr m_ex_ptr = nullptr;
  // 指向的主协程id
  Coroutine *m_master_co;
  // yield的次数
  uint64_t m_yield_cnt = 0;
  // 标记是否为主协程
  bool m_is_master = false;
};

} // namespace src