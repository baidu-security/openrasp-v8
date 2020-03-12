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

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include "queue-request.h"
#include "request.h"

namespace openrasp_v8 {

class RequestQueue {
 public:
  RequestQueue(uint32_t size) : size(size) {}
  bool Append(std::unique_ptr<HTTPRequest> request) {
    std::lock_guard<std::mutex> lock(mtx);
    if (terminated || q.size() >= size) {
      return false;
    }
    q.push(std::move(request));
    cv.notify_one();
    return true;
  }
  std::unique_ptr<HTTPRequest> GetNext() {
    std::unique_lock<std::mutex> lock(mtx);
    for (;;) {
      if (terminated) {
        cv.notify_all();
        return nullptr;
      }
      if (!q.empty()) {
        std::unique_ptr<HTTPRequest> request = std::move(q.front());
        q.pop();
        return request;
      }
      cv.wait(lock);
    }
  }
  void Terminate() {
    std::lock_guard<std::mutex> lock(mtx);
    terminated = true;
    cv.notify_all();
  }
  size_t GetQueuing() {
    std::lock_guard<std::mutex> lock(mtx);
    return q.size();
  }

 private:
  std::condition_variable cv;
  std::mutex mtx;
  std::queue<std::unique_ptr<HTTPRequest>> q;
  uint32_t size;
  bool terminated = false;
};

class WorkerThread : public std::thread {
 public:
  WorkerThread(std::shared_ptr<RequestQueue> q) : std::thread(Run, q) {}
  ~WorkerThread() { join(); }
  static void Run(std::shared_ptr<RequestQueue> q) {
    while (std::unique_ptr<HTTPRequest> request = q->GetNext()) {
      auto response = request->GetResponse();
      if (response.error) {
        Platform::logger(std::string("queue request failed: ") + response.error.message + std::string("\n"));
      } else if (response.status_code != 200) {
        Platform::logger(std::string("queue request status: ") + std::to_string(response.status_code) +
                         std::string(" body: ") + response.text.substr(0, 1024) + std::string("\n"));
      }
    }
  }
};

size_t QueueRequest::pool_size = 1;
size_t QueueRequest::queue_size = 100;

void QueueRequest::Initialize(uint32_t pool_size, uint32_t queue_size) {
  QueueRequest::pool_size = pool_size;
  QueueRequest::queue_size = queue_size;
}

QueueRequest& QueueRequest::GetInstance() {
  static QueueRequest instance(pool_size, queue_size);
  return instance;
}

QueueRequest::QueueRequest(uint32_t pool_size, uint32_t queue_size) {
  q.reset(new RequestQueue(queue_size));
  for (uint32_t i = 0; i < pool_size; i++) {
    pool.emplace_back(new WorkerThread(q));
  }
}

QueueRequest::~QueueRequest() {
  Terminate();
}

void QueueRequest::Terminate() {
  std::lock_guard<std::mutex> lock(mtx);
  if (!terminated) {
    terminated = true;
    q->Terminate();
    pool.clear();
  }
}

bool QueueRequest::Post(std::unique_ptr<HTTPRequest> request) {
  std::lock_guard<std::mutex> lock(mtx);
  if (terminated) {
    return false;
  }
  return q->Append(std::move(request));
}

size_t QueueRequest::GetQueuing() {
  std::lock_guard<std::mutex> lock(mtx);
  return q->GetQueuing();
}

}  // namespace openrasp_v8