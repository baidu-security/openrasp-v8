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
#include <memory>
#include <mutex>
#include <vector>

namespace openrasp_v8 {

class HTTPRequest;
class RequestQueue;
class WorkerThread;
class QueueRequest {
 public:
  QueueRequest(uint32_t pool_size, uint32_t queue_size);
  ~QueueRequest();
  void Terminate();
  bool Post(std::unique_ptr<HTTPRequest> request);
  size_t GetQueuing();

  static void Initialize(uint32_t pool_size, uint32_t queue_size);
  static QueueRequest& GetInstance();

 private:
  std::mutex mtx;
  std::shared_ptr<RequestQueue> q;
  std::vector<std::unique_ptr<WorkerThread>> pool;
  bool terminated = false;

  static size_t pool_size;
  static size_t queue_size;
};

}  // namespace openrasp_v8