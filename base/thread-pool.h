/*
 * Copyright 2017-2019 Baidu Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace openrasp_v8 {
class ThreadPool {
 public:
  typedef std::function<void()> Task;

  ThreadPool(size_t thread_size, size_t queue_cap) : queue_cap(queue_cap) {
    for (size_t i = 0; i < thread_size; i++) {
      threads.emplace_back([this]() {
        while (auto task = GetNext()) {
          task();
        }
      });
    }
  }

  ~ThreadPool() {
    {
      std::lock_guard<std::mutex> lock(mtx);
      terminated = true;
    }
    cv.notify_all();
    for (auto& t : threads) {
      if (t.joinable()) {
        t.join();
      }
    }
  }

  bool Post(Task&& task) {
    {
      std::lock_guard<std::mutex> lock(mtx);
      if (terminated || tasks.size() >= queue_cap) {
        return false;
      }
      tasks.emplace(std::forward<Task>(task));
    }
    cv.notify_one();
    return true;
  }

  Task GetNext() {
    std::unique_lock<std::mutex> lock(mtx);
    for (;;) {
      if (terminated) {
        return nullptr;
      } else if (!tasks.empty()) {
        auto task = std::move(tasks.front());
        tasks.pop();
        return task;
      } else {
        cv.wait(lock);
      }
    }
  }

  size_t GetQueueSize() {
    {
      std::lock_guard<std::mutex> lock(mtx);
      return tasks.size();
    }
  }

 private:
  std::condition_variable cv;
  std::mutex mtx;
  std::vector<std::thread> threads;
  std::queue<Task> tasks;
  size_t queue_cap;
  bool terminated = false;
};
}  // namespace openrasp_v8