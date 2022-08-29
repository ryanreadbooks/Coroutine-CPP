#include <iostream>
#include "containers.hpp"
using namespace ahri;

int main() {
  ThreadSafeDeque<int> d;
  d.PushBack(1);
  d.PushBack(2);
  d.PushBack(3);
  d.PushBack(5);
  d.PushBack(12);

  std::cout << d.Size() << std::endl;

  d.PopBack();
  d.PopFront();
  d.EmplaceBack(45);
  d.EmplaceBack(78);
  d.EmplaceFront(13);
  d.EmplaceFrontUnsafe(9);

  std::cout << d.Size() << std::endl;
  std::cout << d.Front() << std::endl;
  std::cout << d.Back() << std::endl;

  while (!d.Empty()) {
    std::cout << d.Front() << " ";
    d.PopFront();
  }
  std::cout << std::endl;

  d.PushFront(56);
  d.PushBack(89);
  d.PushBack(23);
  d.PushBack(98);
  d.PushBack(223);
  d.PushBack(123);

  for (size_t i = 0; i < d.Size(); ++i) {
    std::cout << d[i] << " ";
  }
  std::cout << std::endl;

  for (auto it = d.begin(); it != d.end(); ++it) {
    std::cout << *it << " ";
  }
  std::cout << std::endl;

  std::cout << "pop Front and Get\n";
  ThreadSafeDeque<int> q;
  d.PopFrontAndReplace(3, q);
  std::cout << "pop 1\n";

  for (auto &num : q) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  std::cout << "pop back and Get\n";
  ThreadSafeDeque<int> w;
  d.PopBackAndReplace(3, w);
  for (auto &num : w) {
    std::cout << num << " ";
  }
  std::cout << std::endl;
  std::cout << "d size = " << d.SizeNoLock() << std::endl;

  std::cout << "-----------------------------\n";

  ThreadSafeDeque<int> ok{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  for (auto &num : ok) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  ThreadSafeDeque<int> ok1;
  ok.PopBackAndAppend(2, ok1);
  ok.PopFrontAndAppend(3, ok1);
  for (auto &num : ok1) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  for (auto &num : ok) {
    std::cout << num << " ";
  }
  std::cout << std::endl;

  return 0;
}