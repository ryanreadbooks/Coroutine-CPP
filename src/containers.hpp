#ifndef __AHRI_CONTAINERS_HPP__
#define __AHRI_CONTAINERS_HPP__

#include <deque>
#include <mutex>
#include <iostream>


using std::deque;
using std::lock_guard;
using std::mutex;

namespace ahri {
/**
 * @brief 带锁的deque
 *
 * @tparam T
 */
template <typename T> 
class ThreadSafeDeque {
public:
  typedef typename deque<T>::iterator d_iterator;
  typedef typename deque<T>::const_iterator const_d_iterator;
  typedef typename deque<T>::reference d_reference;
  typedef typename deque<T>::const_reference const_d_reference;
  typedef typename deque<T>::size_type d_size_type;

  ThreadSafeDeque() {}

  ThreadSafeDeque(std::initializer_list<T> init) : m_datas(init) {}

  bool Empty() const noexcept {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.empty();
  }

  size_t Size() const noexcept {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.size();
  }
  
  size_t SizeNoLock() const noexcept {
    return m_datas.size();
  }

  size_t MaxSize() const noexcept {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.max_size();
  }

  void ClearUnsafe() noexcept {
    m_datas.clear();
  }

  void Clear() noexcept {
    lock_guard<mutex> lk(m_mtx);
    m_datas.clear();
  }

  void PushBack(const T& value) {
    lock_guard<mutex> lk(m_mtx);
    m_datas.push_back(value);
  }

  void PushBack(T&& value) {
    lock_guard<mutex> lk(m_mtx);
    m_datas.push_back(value);
  }

  void PushBackUnsafe(const T& value) {
    m_datas.push_back(value);
  }

  void PushBackUnsafe(T&& value) {
    m_datas.push_back(value);
  }

  d_iterator Erase(d_iterator pos) {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.erase(pos);
  }

  d_iterator Erase(const_d_iterator pos) {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.erase(pos);
  }

  d_iterator Erase(d_iterator first, d_iterator last) {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.erase(first, last);
  }

  d_iterator Erase(const_d_iterator first, const_d_iterator last) {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.erase(first, last);
  }

  d_iterator EraseUnsafe(d_iterator pos) {
    return m_datas.erase(pos);
  }

  d_iterator EraseUnsafe(const_d_iterator pos) {
    return m_datas.erase(pos);
  }

  d_iterator EraseUnsafe(d_iterator first, d_iterator last) {
    return m_datas.erase(first, last);
  }

  d_iterator EraseUnsafe(const_d_iterator first, const_d_iterator last) {
    return m_datas.erase(first, last);
  }

  template <typename... Args>
  void EmplaceBack(Args&&... args) {
    lock_guard<mutex> lk(m_mtx);
    m_datas.emplace_back(args...);
  }

  template <typename... Args>
  void EmplaceBackUnsafe(Args&&... args) {
    m_datas.emplace_back(args...);
  }

  void PopBack() {
    lock_guard<mutex> lk(m_mtx);
    m_datas.pop_back();
  }

  void PopBackUnsafe() {
    m_datas.pop_back();
  }

  void PushFront(const T& value) {
    lock_guard<mutex> lk(m_mtx);
    m_datas.push_front(value);
  }

  void PushFront(T&& value) {
    lock_guard<mutex> lk(m_mtx);
    m_datas.push_front(value);
  }

  void PushFrontUnsafe(const T& value) {
    m_datas.push_front(value);
  }

  void PushFrontUnsafe(T&& value) {
    m_datas.push_front(value);
  }

  void PopFront() {
    lock_guard<mutex> lk(m_mtx);
    m_datas.pop_front();
  }

  void PopFrontUnsafe() {
    m_datas.pop_front();
  }

  template<typename... Args>
  void EmplaceFront(Args&&... args) {
    lock_guard<mutex> lk(m_mtx);
    m_datas.emplace_front(args...);
  }

  template<typename... Args>
  void EmplaceFrontUnsafe(Args&&... args) {
    m_datas.emplace_front(args...);
  }

  void Swap(ThreadSafeDeque& other) {
    lock_guard<mutex> lk(m_mtx);
    m_datas.swap(other.m_datas);
  }

  d_reference Front() {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.front();
  }

  d_reference FrontNoLock() {
    return m_datas.front();
  }

  const_d_reference Front() const {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.front();
  }

  const_d_reference FrontNoLock() const {
    return m_datas.front();
  }

  d_reference Back() {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.back();
  }

  d_reference BackNoLock() {
    return m_datas.back();
  }

  const_d_reference Back() const {
    lock_guard<mutex> lk(m_mtx);
    return m_datas.back();
  }

  const_d_reference BackNoLock() const {
    return m_datas.back();
  }

  /**
   * @brief 取出前面n个元素，并且覆盖ans原本的内容
   * 
   * @param n 数量
   * @return ThreadSafeDeque<T> 
   */
  void PopFrontAndReplace(size_t n, ThreadSafeDeque& ans) {
    lock_guard<mutex> lk(m_mtx);
    PopFrontAndReplaceUnsafe(n, ans);
  }

  void PopFrontAndReplaceUnsafe(size_t n, ThreadSafeDeque& ans) {
    n = std::min(n, m_datas.size());
    auto begin = m_datas.begin();
    auto end = m_datas.begin() + n;
    ans.m_datas.assign(begin, end);
    m_datas.erase(begin, end);
  }

  /**
   * @brief 取出后面n个元素，并且覆盖ans原本的内容
   * 
   * @param n 数量
   * @return ThreadSafeDeque<T> 
   */
  void PopBackAndReplace(size_t n, ThreadSafeDeque& ans) {
    lock_guard<mutex> lk(m_mtx);
    PopBackAndReplaceUnsafe(n, ans);
  }

  void PopBackAndReplaceUnsafe(size_t n, ThreadSafeDeque& ans) {
    n = std::min(n, m_datas.size());
    auto begin = m_datas.begin() + m_datas.size() - n;
    auto end = m_datas.end();
    ans.m_datas.assign(begin, end);
    m_datas.erase(begin, end);
  }

  /**
   * @brief 取出前面n个元素，并且添加到out末尾
   * 
   * @param n 
   * @param out 
   */
  void PopFrontAndAppend(size_t n, ThreadSafeDeque& out) {
    lock_guard<mutex> lk(m_mtx);
    PopFrontAndAppendUnsafe(n, out);
  }

  void PopFrontAndAppendUnsafe(size_t n, ThreadSafeDeque& out) {
    n = std::min(n, m_datas.size());
    auto begin = m_datas.begin();
    auto end = m_datas.begin() + n;
    out.m_datas.insert(out.m_datas.end(), begin, end);
    m_datas.erase(begin, end);
  }

  /**
   * @brief 取出末尾n个元素，并且添加到out末尾
   * 
   * @param n 
   * @param out 
   */
  void PopBackAndAppend(size_t n, ThreadSafeDeque& out) {
    lock_guard<mutex> lk(m_mtx);
    PopBackAndAppendUnsafe(n, out);
  }

  void PopBackAndAppendUnsafe(size_t n, ThreadSafeDeque& out) {
    n = std::min(n, m_datas.size());
    auto begin = m_datas.begin() + m_datas.size() - n;
    auto end = m_datas.end();
    out.m_datas.insert(out.m_datas.end(), begin, end);
    m_datas.erase(begin, end);
  }


/**
 * @brief 迭代器
 * 
 */
public:
  d_iterator begin() noexcept {
    return m_datas.begin();
  }

  const_d_iterator begin() const noexcept {
    return m_datas.begin();
  }

  const_d_iterator cbegin() const noexcept {
    return m_datas.cbegin();
  }

  d_iterator end() noexcept {
    return m_datas.end();
  }

  const_d_iterator end() const noexcept {
    return m_datas.end();
  }

  const_d_iterator cend() const noexcept {
    return m_datas.cend();
  }

  d_reference operator[](d_size_type pos) {
    return m_datas[pos];
  }

  const_d_reference operator[](d_size_type pos) const {
    return m_datas[pos];
  }

public:
  mutex &LockRef() const { return m_mtx; }

private:
  deque<T> m_datas;
  mutable mutex m_mtx;
};

} // namespace src

#endif