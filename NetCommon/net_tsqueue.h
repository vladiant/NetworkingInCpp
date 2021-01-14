#pragma once

#include "net_common.h"

namespace olc {
namespace net {

template <typename T>
class tsqueue {
 public:
  tsqueue() = default;
  tsqueue(const tsqueue<T>&) = delete;

  virtual ~tsqueue() { clear(); }

  // Returns and maintains item at front of Queue
  const T& front() {
    std::lock_guard<std::mutex> lock(muxQueue);
    return deqQueue.front();
  }

  // Returns and maintains item at back of Queue
  const T& back() {
    std::lock_guard<std::mutex> lock(muxQueue);
    return deqQueue.back();
  }

  // Add an item to the back of Queue
  void push_back(const T& item) {
    std::lock_guard<std::mutex> lock(muxQueue);
    deqQueue.emplace_back(std::move(item));

    // TODO: Too many locks - rework!
    std::unique_lock<std::mutex> u_lock(muxBlocking);
    cvBlocking.notify_one();
  }

  // Add an item to the front of Queue
  void push_front(const T& item) {
    std::lock_guard<std::mutex> lock(muxQueue);
    deqQueue.emplace_front(std::move(item));

    // TODO: Too many locks - rework!
    std::unique_lock<std::mutex> u_lock(muxBlocking);
    cvBlocking.notify_one();
  }

  // Return true if Queue has no items
  bool empty() {
    std::lock_guard<std::mutex> lock(muxQueue);
    return deqQueue.empty();
  }

  // Return number of items in the Queue
  bool count() {
    std::lock_guard<std::mutex> lock(muxQueue);
    return deqQueue.size();
  }

  // Clears the Queue
  void clear() {
    std::lock_guard<std::mutex> lock(muxQueue);
    return deqQueue.clear();
  }

  // Removes and returns item from front of Queue
  // TODO: Fix the case for empty queue
  T pop_front() {
    std::lock_guard<std::mutex> lock(muxQueue);
    auto t = std::move(deqQueue.front());
    deqQueue.pop_front();
    return t;
  }

  // Removes and returns item from back of Queue
  // TODO: Fix the case for empty queue
  T pop_back() {
    std::lock_guard<std::mutex> lock(muxQueue);
    auto t = std::move(deqQueue.back());
    deqQueue.pop_back();
    return t;
  }

  void wait() {
    // TODO: Rework without loop
    while (empty()) {
      std::unique_lock<std::mutex> lock(muxBlocking);
      cvBlocking.wait(lock);
    }
  }

 protected:
  std::mutex muxQueue;
  std::deque<T> deqQueue;

  std::condition_variable cvBlocking;
  std::mutex muxBlocking;
};

}  // namespace net
}  // namespace olc
