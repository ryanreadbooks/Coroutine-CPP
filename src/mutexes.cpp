#include "utils.h"
#include "mutexes.h"

namespace ahri {
  
Semaphore::Semaphore(uint32_t count) {
  if (sem_init(&m_semaphore, 0, count)) {
    THROW_SYS_ERROR("sem_init error");
  }
}

Semaphore::~Semaphore() {
  sem_destroy(&m_semaphore);
}

void Semaphore::Wait() {
  if (sem_wait(&m_semaphore)) {
    THROW_SYS_ERROR("sem_wait error");
  }
}

void Semaphore::Post() {
  if (sem_post(&m_semaphore)) {
    THROW_SYS_ERROR("sem_post error");
  }
}

} // namespace src
