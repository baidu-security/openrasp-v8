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

#include <chrono>
#include <mutex>
#include <thread>
#include "bundle.h"

namespace openrasp_v8 {
TimeoutTask::TimeoutTask(Isolate* isolate, std::future<void> fut, int milliseconds)
    : isolate(isolate),
      fut(std::move(fut)),
      time_point(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds)) {}

void TimeoutTask::Run() {
  if (std::future_status::timeout == fut.wait_until(time_point)) {
    isolate->GetData()->is_timeout = true;
    isolate->TerminateExecution();
  }
}
}  // namespace openrasp_v8