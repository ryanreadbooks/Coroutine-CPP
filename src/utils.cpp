#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <sys/time.h>
#include <sstream>

#include "utils.h"
#include "thread.h"
#include "coroutine.h"
#include "coexecutor.h"


namespace ahri {

std::string GetBackTraceStr(int level, int skip) {
  void **buffer = (void **) malloc(sizeof(void *) * level);
  size_t n = backtrace(buffer, level);

  char **strings = backtrace_symbols(buffer, n);
  if (strings == nullptr) {
    return "";
  }
  std::stringstream ss;
  for (size_t i = skip; i < n; ++i) {
    ss << "(" << i << ")";
    for (size_t j = 0; j <= i; ++j) {
      ss << " ";
    }
    ss << strings[i] << std::endl;
  }

  free(strings);
  free(buffer);
  return ss.str();
}

uint32_t GetElapse() {
  return clock() / CLOCKS_PER_SEC;
}

int32_t GetThreadId() {
  return syscall(SYS_gettid);
}

int64_t GetCoroutineId() {
  return this_coroutine::GetId();
  // return -1;
}

std::string GetThreadName() {
  return Thread::GetName();
}

uint64_t GetCurrentTimestamp() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec;
}

uint64_t GetCurrentMs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUs() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

std::string StringUtils::RightTrim(const std::string &str, const std::string &delim) {
  auto end = str.find_last_not_of(delim);
  if (end == std::string::npos) {
    return "";
  }
  return str.substr(0, end);
}

std::string StringUtils::LeftTrim(const std::string &str, const std::string &delim) {
  auto begin = str.find_first_not_of(delim);
  if (begin == std::string::npos) {
    return "";
  }
  return str.substr(begin);
}

std::string StringUtils::Trim(const std::string &str, const std::string &delim) {
  auto begin = str.find_first_not_of(delim);
  if (begin == std::string::npos) {
    return "";
  }
  auto end = str.find_last_not_of(delim);
  return str.substr(begin, end - begin + 1);
}

void StringUtils::ToLower(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

void StringUtils::ToUpper(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

void FileUtils::ListAllFiles(std::vector<std::string> &files, const std::string &path, const std::string &prefix) {

}

static int __lstat(const char *file, struct stat *st = nullptr) {
  struct stat lst;
  // 成功返回0，失败返回-1
  int ret = lstat(file, &lst);
  if (st) {
    *st = lst;
  }
  return ret;
}

static int __mkdir(const char *dirname) {
  // 先检查能否访问此路径，成功则会返回0，失败则表明路径不存在，需要创建
  if (access(dirname, F_OK) == 0) {
    return 0;
  }
  // 成功返回0，失败返回-1。默认775权限
  return mkdir(dirname, 0775);
}

bool FileUtils::MakeDir(const std::string &dirname) {
  if (__lstat(dirname.c_str()) == 0) {
    return true;  // 路径已经存在
  }
  // 路径不存在，创建它
  char *path = strdup(dirname.c_str());
  char *ptr = strchr(path + 1, '/');
  do {
    for (; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
      *ptr = '\0';
      if (__mkdir(path) != 0) {
        break;
      }
    }
    if (ptr != nullptr) {
      break;
    } else if (__mkdir(path) != 0) {
      break;
    }
    free(path);
    return true;
  } while (0);
  free(path);
  return false;
}

bool FileUtils::Rm(const std::string &path) {
  return false;
}

bool FileUtils::Mv(const std::string &from, const std::string &to) {
  return false;
}

std::string FileUtils::DirName(const std::string &filename) {
  if (filename.empty()) {
    return ".";
  }
  auto pos = filename.rfind('/');
  if (pos == 0) {
    return "/";
  } else if (pos == std::string::npos) {
    return ".";
  }
  return filename.substr(0, pos);
}

std::string FileUtils::BaseName(const std::string &filename) {
  if (filename.empty()) {
    return filename;
  }
  auto pos = filename.rfind('/');
  if (pos == std::string::npos) {
    return filename;
  }
  return filename.substr(pos + 1);
}

bool FileUtils::OpenForRead(std::ifstream &ifs, const std::string &filename, std::ios_base::openmode mode) {
  ifs.open(filename, mode);
  return ifs.is_open();
}

bool FileUtils::OpenForWrite(std::ofstream &ofs, const std::string &filename, std::ios_base::openmode mode) {
  ofs.open(filename, mode);
  if (!ofs.is_open()) {
    // 创建目录再打开
    std::string dir = DirName(filename);
    MakeDir(dir);
    ofs.open(filename, mode);
  }
  return ofs.is_open();
}


}// namespace src