#pragma once

#include <cxxabi.h> // to Get readable typename
#include <execinfo.h> // for backtrace
#include <assert.h> // for assert
#include <errno.h>
#include <system_error>
#include <string>
#include <vector>
#include <iostream>
#include <unistd.h>
#include <dirent.h>

// 断言宏
#define AHRI_ASSERT(condition) \
  if (!(condition)) { \
    std::cerr << "ASSERTION '" << #condition << "' failed " \
        << "\ncall backtrace: \n" \
        << ahri::GetBackTraceStr(100); \
    assert(condition); \
  }

// 带参数的断言宏
#define AHRI_ASSERT_MSG(condition, msg) \
  if (!(condition)) { \
    std::cerr << "ASSERTION '" << #condition << "' failed " \
        << "\n" << msg \
        << "\ncall backtrace: \n" \
        << ahri::GetBackTraceStr(100); \
    assert(condition); \
  }

// 定义抛出system_error的宏函数，msg指定抛出异常的信息
#define THROW_SYS_ERROR(msg) \
    throw std::system_error(errno, std::generic_category(), msg)

namespace ahri {

/**
 * @brief 获取类型名
 * 
 * @tparam T 
 * @return std::string 
 */
template <typename T>
inline std::string Typeid2Name() {
  // __cxa_demangle函数返回的空间需要调用者手动释放
  char* str = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
  std::string ret(str);
  free(str);
  return ret;
}

/**
 * @brief 获取变量的类型名
 * 
 * @tparam T 
 * @param val 
 * @return std::string 
 */
template <typename T>
inline std::string Typeid2Name(const T& val) {
  char* str = abi::__cxa_demangle(typeid(val).name(), nullptr, nullptr, nullptr);
  std::string ret(str);
  free(str);
  return ret;
}

/**
 * @brief 获取函数调用栈，返回字符串
 * 
 * @param level 返回最大的数量
 * @return std::string 结果字符串
 */
std::string GetBackTraceStr(int level = 100, int skip = 1);

/**
 * @brief 获取程序启动时间
 * 
 * @return uint32_t 
 */
uint32_t GetElapse();

/**
 * 获取线程id
 */
int32_t GetThreadId();

/**
 * @brief 获取协程id
 * 
 * @return int32_t 
 */
int64_t GetCoroutineId();

/**
 * 获取线程名称
 */ 
std::string GetThreadName();

/**
 * Get current timestamp in second
 * @return
 */
uint64_t GetCurrentTimestamp();

/**
 * @brief 获取当前毫秒
 * 
 * @return uint64_t 
 */
uint64_t GetCurrentMs();

/**
 * @brief 获取当前微秒
 * 
 * @return uint64_t 
 */
uint64_t GetCurrentUs();

/**
 * @brief 字符串帮助类
 * 
 */
class StringUtils {
public:
  static std::string LeftTrim(const std::string& str, const std::string& delim = " \t\r\n");
  static std::string RightTrim(const std::string& str, const std::string& delim = " \t\r\n");
  static std::string Trim(const std::string& str, const std::string& delim = " \t\r\n");
  static void ToLower(std::string& str);
  static void ToUpper(std::string& str);
};

/**
 * @brief 文件帮助类
 * 
 */
class FileUtils {
public:
  static void ListAllFiles(std::vector<std::string> & files, const std::string& path, const std::string& prefix);
  
  static bool MakeDir(const std::string& dirname);

  static bool Rm(const std::string& path);

  static bool Mv(const std::string& from, const std::string& to);

  /**
   * 返回路径名
   */
  static std::string DirName(const std::string& filename);

  /**
   * 返回文件名 
   */ 
  static std::string BaseName(const std::string& filename);

  /**
   * 打开文件用以读
   */
  static bool OpenForRead(std::ifstream& ifs, const std::string& filename, std::ios_base::openmode mode);

  /**
   * 打开文件用以写，路径不存在则先创建
   */
  static bool OpenForWrite(std::ofstream& ofs, const std::string& filename, std::ios_base::openmode mode);
};

} // namespace src