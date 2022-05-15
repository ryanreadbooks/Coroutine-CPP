#ifndef __AHRI_SINGLETON_HPP__
#define __AHRI_SINGLETON_HPP__

#include <memory>

namespace ahri {

/**
 * @brief 单例模式
 * 
 * @tparam T 
 */
template <typename T>
class Singleton {
public:
  static T& GetInstance() {
    static T t;
    return t;
  }
};

template <typename T>
class SingletonPtr {
public:
  static std::shared_ptr<T> GetInstance() {
    static std::shared_ptr<T> t(new T);
    return t;
  }
};

} // namespace src

#endif