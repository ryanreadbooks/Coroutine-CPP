#include "coroutine.h"

namespace ahri {

struct MemAllocator {
  static void *alloc(size_t s) {
    return (void *) malloc(s);
  }

  static void dealloc(void *where, size_t s) {
    free(where);
  }
};

namespace coroutine_meta {
// 协程自增id
static std::atomic<int64_t> s_co_id{-1};
// 当前协程数量
static std::atomic<size_t> s_co_count{0};

size_t GetCoCount() {
  return s_co_count;
}

int64_t __get_co_gl_id() {
  return s_co_count;
}

}; // namespace CoroutineMeta


std::string status2string(Coroutine::Status st) {
  switch (st) {
    case Coroutine::Status::IDLE:
      return "IDLE";
      break;
    case Coroutine::Status::RUNNING:
      return "RUNNING";
      break;
    case Coroutine::Status::FINISHED:
      return "FINISHED";
      break;
    case Coroutine::Status::HOLD:
      return "HOLD";
      break;
    case Coroutine::Status::EXCEPT:
      return "EXCEPT";
      break;
    default:
      return "IDLE";
      break;
  }
}

Coroutine::Coroutine()
    : m_stacksize(0),
      m_stack(nullptr),
      m_id(++coroutine_meta::s_co_id),
      m_func(nullptr) {
  if (getcontext(&m_ctx) != 0) {
    std::cerr << "Can not construct master coroutine instance when getcontext"
              << " for thread-" << GetThreadId() << std::endl;
    std::cerr << "Can not construct master coroutine instance when getcontext"
              << " for thread-" << GetThreadId() << std::endl;
    int ret = -1;
    pthread_exit(&ret);
  }
  // 创建成功
  ++coroutine_meta::s_co_count;
  std::cout << "Master coroutine(id=" << m_id
            << ") created for thread-" << GetThreadId() << std::endl;
  m_is_master = true;
}

Coroutine::Ptr Coroutine::CreateMaster() {
  return std::make_shared<Coroutine>();
}

Coroutine::Coroutine(const Executable &executable, size_t ss)
    : m_stacksize(ss),
      m_stack(nullptr),
      m_id(++coroutine_meta::s_co_id),
      m_func(std::move(executable)) {
  // AHRI_ASSERT_MSG(master != nullptr, "Master coroutine can not be null");
  // 栈大小为0表示使用默认大小
  m_stacksize = m_stacksize == 0 ? DEFAULT_STACK_SIZE : m_stacksize;
  // 初始化ucontext_t
  if (getcontext(&m_ctx) != 0) {
    THROW_SYS_ERROR("Can not construct coroutine instance when getcontext");
  }
  ++coroutine_meta::s_co_count;
  std::cout << "Coroutine-" << m_id << " constructed in thread-" << GetThreadId() << std::endl;
}

Coroutine::~Coroutine() {
  // 释放占用的栈空间
  if (m_stack) {
    // 非IDLE，FINISHED，EXCEPE三种情况之一，不能够析构
    AHRI_ASSERT_MSG(CanDestroy(),
                    "Coroutine can not be destroyed when not in IDLE, FINISHED or EXCEPT. Current status is " + status2string(m_status))
    MemAllocator::dealloc(m_stack, m_stacksize);
    m_stack = nullptr;
  }
  std::string s = m_is_master ? "(master)" : "";
#ifdef DEV_DEBUGGING
  std::cout << "Coroutine-" << m_id
            << s << " got destructed in thread-" << ahri::GetThreadId() << std::endl;
#endif
  --coroutine_meta::s_co_count;
}

std::string Coroutine::GetStatusAsString() const {
  return status2string(m_status);
}

void Coroutine::Resume() {
  // 已经完成了也不用resume
  if (m_status == FINISHED || m_status == EXCEPT) {
    return;
  }
  // 非IDLE或HOLD之一，不能resume
  AHRI_ASSERT_MSG(CanResume(),
                  "Coroutine can not resume when not in IDLE, HOLD. Current status is " + status2string(m_status));
  // 检查是否已经初始化
  if (!m_is_master && !m_stack) {
    // 设置栈
    m_stack = MemAllocator::alloc(m_stacksize);
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;
    m_ctx.uc_link = &(ahri::CoExecutor::GetMasterCo()->m_ctx); // 执行完后自动切回当前线程的主协程中
    uintptr_t ptr = (uintptr_t) this;
    // 拿到地址的低32位和高32位
    // uint32_t会截断高32位
    makecontext(&m_ctx, (void (*)(void)) StaticRun, 2, (uint32_t) ptr, (uint32_t) (ptr >> 32));
    m_master_co = CoExecutor::GetMasterCo();
  }
  // 换入
  SetStatus(RUNNING);
  // 主协程作为切换的中介
  swapcontext(&m_master_co->m_ctx, &m_ctx);
}

void Coroutine::GiveUp() {
  SetStatus(HOLD);
  ++m_yield_cnt;
  swapcontext(&m_ctx, &m_master_co->m_ctx);
}

void Coroutine::Run() {
  std::cout << "Coroutine-" << m_id << " (thread-" << GetThreadId() << ") starts running...\n";
  // 当前协程运行
  try {
    m_func();
    SetStatus(FINISHED);
    m_func = nullptr;
  }
  catch (...) { // 发生异常
    SetStatus(EXCEPT);
    m_ex_ptr = std::current_exception();
  }
  // 运行完成，换出此协程，并且换入主协程（上面设置了uc_link，所以完成后会自动切换）
  std::cout << "Coroutine-" << m_id << " (thread-" << GetThreadId() << ") ends running..." << std::endl;
}

void Coroutine::StaticRun(uint32_t low32, uint32_t high32) {
  // 获取当前调用的对象
  uintptr_t sptr = (uintptr_t) low32 | ((uintptr_t) high32 << 32);
  Coroutine &self = *(Coroutine * )(sptr);
  self.Run();
  self.m_func = std::function<void()>();  // 清理function对象
}

} // namespace src