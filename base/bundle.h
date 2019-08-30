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
#include <future>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace openrasp {

// Only accept Latin-1
inline v8::Local<v8::String> NewV8Key(v8::Isolate* isolate, const char* str, size_t len = -1) {
  auto data = reinterpret_cast<const uint8_t*>(str);
  return v8::String::NewFromOneByte(isolate, data, v8::NewStringType::kInternalized, len)
      .FromMaybe(v8::String::Empty(isolate));
}

// Only accept Latin-1
inline v8::Local<v8::String> NewV8Key(v8::Isolate* isolate, const std::string& str) {
  return NewV8Key(isolate, str.c_str(), str.length());
}

inline v8::Local<v8::String> NewV8String(v8::Isolate* isolate, const char* str, size_t len = -1) {
  return v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal, len).FromMaybe(v8::String::Empty(isolate));
}

inline v8::Local<v8::String> NewV8String(v8::Isolate* isolate, const std::string& str) {
  return NewV8String(isolate, str.c_str(), str.length());
}

class Exception : public std::string {
 public:
  Exception(v8::Isolate* isolate, v8::TryCatch& try_catch);
};

typedef void (*Logger)(const std::string& message);

class Platform : public v8::Platform {
 public:
  explicit Platform(int thread_pool_size);
  ~Platform() override;

  // v8::platform::DefaultPlatform implementation.
  bool PumpMessageLoop(v8::Isolate* isolate,
                       v8::platform::MessageLoopBehavior behavior = v8::platform::MessageLoopBehavior::kDoNotWait);
  // void RunIdleTasks(v8::Isolate* isolate, double idle_time_in_seconds);

  // v8::Platform implementation.
  int NumberOfWorkerThreads() override;
  std::shared_ptr<v8::TaskRunner> GetForegroundTaskRunner(v8::Isolate* isolate) override;
  void CallOnWorkerThread(std::unique_ptr<v8::Task> task) override;
  void CallDelayedOnWorkerThread(std::unique_ptr<v8::Task> task, double delay_in_seconds) override;
  void CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task) override;
  void CallDelayedOnForegroundThread(v8::Isolate* isolate, v8::Task* task, double delay_in_seconds) override;
  // void CallIdleOnForegroundThread(v8::Isolate* isolate, v8::IdleTask* task) override;
  // bool IdleTasksEnabled(v8::Isolate* isolate) override;
  double MonotonicallyIncreasingTime() override;
  double CurrentClockTimeMillis() override;
  v8::TracingController* GetTracingController() override;
  v8::Platform::StackTracePrinter GetStackTracePrinter() override;
  // v8::PageAllocator* GetPageAllocator() override;

  static Platform* New(int thread_pool_size);
  static Platform* Get();
  void Startup();
  void Shutdown();

  static Logger logger;

 private:
  static std::unique_ptr<Platform> instance;
  int thread_pool_size;
  std::unique_ptr<v8::Platform> default_platform;
  std::unique_ptr<v8::platform::tracing::TracingController> tracing_controller;
};

class TimeoutTask : public v8::Task {
 public:
  TimeoutTask(v8::Isolate* isolate, std::future<void> fut, int milliseconds = 100);
  void Run() override;

 private:
  v8::Isolate* isolate;
  std::future<void> fut;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
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
  v8::Persistent<v8::Context> context;
  v8::Persistent<v8::Object> RASP;
  v8::Persistent<v8::Function> check;
  v8::Persistent<v8::Function> console_log;
  v8::Persistent<v8::Object> request_context;
  v8::Persistent<v8::ObjectTemplate> request_context_templ;
  uint64_t timestamp = 0;
  void* custom_data = nullptr;
};

class Snapshot : public v8::StartupData {
 public:
  uint64_t timestamp = 0;
  static intptr_t* external_references;
  Snapshot() = delete;
  Snapshot(const char* data, size_t raw_size, uint64_t timestamp);
  Snapshot(const std::string& path, uint64_t timestamp);
  Snapshot(const std::string& config,
           const std::vector<PluginFile>& plugin_list,
           const std::string& version,
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
  Isolate() = delete;
  ~Isolate() = delete;
  void Initialize();
  IsolateData* GetData();
  void SetData(IsolateData* data);
  void Dispose();
  bool IsExpired(uint64_t timestamp);
  v8::Local<v8::Array> Check(v8::Local<v8::String> type,
                             v8::Local<v8::Object> params,
                             v8::Local<v8::Object> context,
                             int timeout = 100);
  v8::MaybeLocal<v8::Value> ExecScript(const std::string& source, const std::string& filename, int line_offset = 0);
  v8::MaybeLocal<v8::Value> ExecScript(v8::Local<v8::String> source,
                                       v8::Local<v8::String> filename,
                                       v8::Local<v8::Integer> line_offset);
  v8::MaybeLocal<v8::Value> Log(v8::Local<v8::Value> value);
  static size_t NearHeapLimitCallback(void* data, size_t current_heap_limit, size_t initial_heap_limit);
  static void FatalErrorCallback(const char* location, const char* message);
};

inline bool Initialize(size_t pool_size, Logger logger) {
  const char* flags = std::getenv("OPENRASP_V8_OPTIONS");
  if (flags) {
    v8::V8::SetFlagsFromString(flags, strlen(flags));
  }
  Platform::logger = logger;
  v8::V8::InitializePlatform(Platform::New(pool_size));
  v8::V8::SetDcheckErrorHandler([](const char* file, int line, const char* message){
    std::string msg = "\nDebug check failed: ";
    msg += message;
    msg += ", in ";
    msg += file;
    msg += ", line ";
    msg += std::to_string(line);
    msg += "\n";
    Platform::logger(msg);
    printf("%s", msg.c_str());
  });
  return v8::V8::Initialize();
}

inline bool Dispose() {
  bool rst = v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  return rst;
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