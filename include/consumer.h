#pragma once

#include "logger.h"
#include <boost/lockfree/spsc_queue.hpp>
#include <thread>

template <typename V,
          typename = std::enable_if_t<std::is_same_v<V, std::string> ||
                                      std::is_arithmetic_v<V>>>
class Consumer {
public:
  explicit Consumer(size_t size = 1024)
      : queue_(size), worker_(&Consumer::start, this) {}
  ~Consumer() { worker_.join(); }
  void eat(V v) { queue_.push(v); }

private:
  std::thread worker_;
  boost::lockfree::spsc_queue<V> queue_;
  void start() {
    while (true) {
      V v;
      if (queue_.pop(v)) {
        Logger::instance().info("Consumer: " + v);
      }
    }
  }
};
