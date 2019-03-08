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

#include "_cgo_export.h"
#include "base/bundle.h"
#include "export.h"

extern bool is_initialized;
extern openrasp::Snapshot* snapshot;
extern std::mutex mtx;
extern std::vector<openrasp::PluginFile> plugin_list;
class CustomData {
 public:
  void* context_getters = nullptr;
};
inline CustomData* GetCustomData(openrasp::IsolateData* data) {
  return reinterpret_cast<CustomData*>(data->custom_data);
}
inline CustomData* GetCustomData(openrasp::Isolate* isolate) {
  return GetCustomData(isolate->GetData());
}
class IsolateDeleter {
 public:
  void operator()(openrasp::Isolate* isolate) {
    delete GetCustomData(isolate);
    isolate->Dispose();
  }
};
typedef std::unique_ptr<openrasp::Isolate, IsolateDeleter> IsolatePtr;
inline openrasp::Isolate* GetIsolate() {
  static thread_local IsolatePtr isolate_ptr;
  auto isolate = isolate_ptr.get();
  if (snapshot) {
    if (!isolate || isolate->IsExpired(snapshot->timestamp)) {
      std::unique_lock<std::mutex> lock(mtx, std::try_to_lock);
      if (lock) {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        isolate = openrasp::Isolate::New(snapshot, millis);
        isolate->GetData()->custom_data = new CustomData();
        isolate_ptr.reset(isolate);
      }
    }
  }
  return isolate;
}