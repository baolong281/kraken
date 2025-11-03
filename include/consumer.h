#pragma once

#include <boost/lockfree/spsc_queue.hpp>
#include <thread>

template <typename V> class Consumer {
public:
  Consumer(size_t size = 1024, std::function<void(V)> callback = nullptr)
      : queue_(size), worker_(&Consumer::start, this), callback_(callback) {}
  ~Consumer() { worker_.join(); }
  void eat(V v) { queue_.push(v); }

private:
  std::function<void(V)> callback_;
  std::thread worker_;
  boost::lockfree::spsc_queue<V> queue_;
  void start() {
    while (true) {
      V v;
      if (queue_.pop(v)) {
        callback_(v);
      }
    }
  }
};
