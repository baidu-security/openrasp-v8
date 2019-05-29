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

#undef COMPILER  // mabey conflict with v8 defination
#include <libplatform/libplatform.h>
#include <v8-platform.h>
#include <v8.h>
#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace openrasp {
inline v8::Local<v8::String> NewV8String(v8::Isolate* isolate, const char* str, size_t len = -1) {
  return v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal, len).ToLocalChecked();
}

inline v8::Local<v8::String> NewV8String(v8::Isolate* isolate, const std::string& str) {
  return NewV8String(isolate, str.c_str(), str.length());
}

class Exception : public std::string {
 public:
  Exception(v8::Isolate* isolate, v8::TryCatch& try_catch);
};

class Platform : public v8::Platform {
 public:
  explicit Platform(int thread_pool_size);
  ~Platform() override;

  // v8::platform::DefaultPlatform implementation.
  bool PumpMessageLoop(v8::Isolate* isolate,
                       v8::platform::MessageLoopBehavior behavior = v8::platform::MessageLoopBehavior::kDoNotWait);
  void RunIdleTasks(v8::Isolate* isolate, double idle_time_in_seconds);

  // v8::Platform implementation.
  int NumberOfWorkerThreads() override;
  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner(v8::Isolate* isolate) override;
  void CallOnWorkerThread(std::unique_ptr<v8::Task> task) override;
  void CallDelayedOnWorkerThread(std::unique_ptr<v8::Task> task, double delay_in_seconds) override;
  void CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task) override;
  void CallDelayedOnForegroundThread(v8::Isolate* isolate, v8::Task* task, double delay_in_seconds) override;
  void CallIdleOnForegroundThread(v8::Isolate* isolate, v8::IdleTask* task) override;
  bool IdleTasksEnabled(v8::Isolate* isolate) override;
  double MonotonicallyIncreasingTime() override;
  double CurrentClockTimeMillis() override;
  v8::TracingController* GetTracingController() override;
  v8::Platform::StackTracePrinter GetStackTracePrinter() override;
  v8::PageAllocator* GetPageAllocator() override;

  static Platform* New(int thread_pool_size);
  static Platform* Get();
  void Startup();
  void Shutdown();

 private:
  static std::unique_ptr<Platform> instance;
  int thread_pool_size;
  std::unique_ptr<v8::Platform> default_platform;
  std::unique_ptr<v8::platform::tracing::TracingController> tracing_controller;
};

class TimeoutTask : public v8::Task {
 public:
  TimeoutTask(v8::Isolate* _isolate, int _milliseconds = 100);
  void Run() override;
  std::timed_mutex& GetMtx();
  bool IsTimeout() { return is_timeout; }

 private:
  v8::Isolate* isolate;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
  std::timed_mutex mtx;
  bool is_timeout = false;
};

class PluginFile {
 public:
  PluginFile(const std::string& filename, const std::string& source) : filename(filename), source(source){};
  std::string filename;
  std::string source;
};

class IsolateData {
 public:
  v8::Isolate::CreateParams create_params;
  v8::Persistent<v8::Object> RASP;
  v8::Persistent<v8::Function> check;
  v8::Persistent<v8::Object> request_context;
  v8::Persistent<v8::ObjectTemplate> request_context_templ;
  v8::Persistent<v8::String> key_action;
  v8::Persistent<v8::String> key_message;
  v8::Persistent<v8::String> key_name;
  v8::Persistent<v8::String> key_confidence;
  v8::Persistent<v8::String> key_algorithm;
  v8::Persistent<v8::Function> console_log;
  int action_hash_ignore = 0;
  int action_hash_log = 0;
  int action_hash_block = 0;
  uint64_t timestamp = 0;
  void* custom_data = nullptr;
};

class Snapshot : public v8::StartupData {
 public:
  uint64_t timestamp = 0;
  static intptr_t external_references[3];
  Snapshot() = delete;
  Snapshot(const char* data, size_t raw_size, uint64_t timestamp);
  Snapshot(const std::string& path, uint64_t timestamp);
  Snapshot(const std::string& config,
           const std::vector<PluginFile>& plugin_list,
           uint64_t timestamp,
           void* custom_data = nullptr);
  ~Snapshot();
  bool Save(const std::string& path) const;  // check errno when return value is false
  bool IsOk() const { return data && raw_size; };
  bool IsExpired(uint64_t timestamp) const { return timestamp > this->timestamp; };
};

class Isolate : public v8::Isolate {
 public:
  static Isolate* New(Snapshot* snapshot_blob, uint64_t timestamp);
  IsolateData* GetData();
  void SetData(IsolateData* data);
  void Dispose();
  bool IsExpired(uint64_t timestamp);
  v8::Local<v8::Array> Check(v8::Local<v8::String> type,
                             v8::Local<v8::Object> params,
                             v8::Local<v8::Object> context,
                             int timeout = 100);
  static v8::MaybeLocal<v8::Value> ExecScript(Isolate* isolate,
                                              std::string source,
                                              std::string filename,
                                              int line_offset = 0);
  v8::MaybeLocal<v8::Value> ExecScript(std::string source, std::string filename, int line_offset = 0);
};

// To be implemented Start
v8::Local<v8::ObjectTemplate> CreateRequestContextTemplate(Isolate* isolate);
void plugin_info(Isolate* isolate, const std::string& message);
void alarm_info(Isolate* isolate,
                v8::Local<v8::String> type,
                v8::Local<v8::Object> params,
                v8::Local<v8::Object> result);
// To be implemented End

inline void plugin_info(Isolate* isolate, v8::Local<v8::Value> value) {
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  auto console_log = isolate->GetData()->console_log.Get(isolate);
  (void)console_log->Call(context, console_log, 1, &value).IsEmpty();
}
}  // namespace openrasp

#ifdef UNLIKELY
#undef UNLIKELY
#endif
#ifdef LIKELY
#undef LIKELY
#endif
#if defined(__GNUC__) || defined(__clang__)
#define UNLIKELY(condition) (__builtin_expect(!!(condition), 0))
#define LIKELY(condition) (__builtin_expect(!!(condition), 1))
#else
#define UNLIKELY(condition) (condition)
#define LIKELY(condition) (condition)
#endif