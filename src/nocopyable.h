#pragma once

namespace ahri {

class NoCopyable {
public:
  NoCopyable() = default;

  NoCopyable(const NoCopyable&) = delete;

  NoCopyable& operator=(const NoCopyable&) = delete;
};

} // namespace src
