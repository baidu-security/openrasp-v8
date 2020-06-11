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
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

namespace openrasp_v8 {

constexpr int max_buffer_size = 4 * 1024 * 1024;

// 作为key的string会直接在旧生代创建
inline v8::Local<v8::String> NewV8Key(v8::Isolate* isolate, const char* str, int len = -1) {
  return v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kInternalized, std::min(max_buffer_size, len))
      .FromMaybe(v8::String::Empty(isolate));
}

inline v8::Local<v8::String> NewV8Key(v8::Isolate* isolate, const std::string& str) {
  return NewV8Key(isolate, str.c_str(), str.length());
}

// 普通string，创建在新生代
inline v8::Local<v8::String> NewV8String(v8::Isolate* isolate, const char* str, int len = -1) {
  return v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal, std::min(max_buffer_size, len))
      .FromMaybe(v8::String::Empty(isolate));
}

inline v8::Local<v8::String> NewV8String(v8::Isolate* isolate, const std::string& str) {
  return NewV8String(isolate, str.c_str(), str.length());
}

// 提取v8的exception，格式化成string
class Exception : public std::string {
 public:
  Exception(v8::Isolate* isolate, v8::TryCatch& try_catch);
};

// 日志回调
typedef void (*Logger)(const std::string& message);
// 内存压力回调
typedef bool (*CriticalMemoryPressureCallback)(size_t length);

// v8::Platform为v8运行时提供平台相关的接口，如时钟，线程池等
// v8提供了默认的platform，但是不支持在运行时关闭后台线程池，在php
// fork时会出异常，所以重新包装默认platform，提供开关线程池等操作 下面一些方法被注释是因为有默认值，不需重新实现
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
  // bool OnCriticalMemoryPressure(size_t length) override;

  static Platform* New(int thread_pool_size);
  static Platform* Get();
  void Startup();
  void Shutdown();

  static Logger logger;
  // static CriticalMemoryPressureCallback criticalMemoryPressureCallback;

 private:
  static std::unique_ptr<Platform> instance;
  int thread_pool_size;
  std::unique_ptr<v8::Platform> default_platform;
  std::unique_ptr<v8::platform::tracing::TracingController> tracing_controller;
};

// 插件文件对象
class PluginFile {
 public:
  PluginFile(const std::string& filename, const std::string& source) : filename(filename), source(source){};
  std::string filename;
  std::string source;
};

// v8::Isolate只能通过v8::Isolate::New工厂函数创建，所以下面v8::Isolate的子类openrasp::Isolate不能增加对象
// 只能通过v8::Isolate::SetData方法向其对象绑定数据
// IsolateData是需要绑定到Isolate的数据的集合
// v8::Persistent和v8::Local都是js对象的容器，区别是作用域，操作v8::Persistent的对象时必须先传换成v8::Local
class IsolateData {
 public:
  v8::Isolate::CreateParams create_params;     // v8::Isolate::New的参数
  v8::Persistent<v8::Context> context;         // v8的context对象，执行js代码需要在一个v8 context中
  v8::Persistent<v8::Object> RASP;             // js中RASP类
  v8::Persistent<v8::Function> check;          // 缓存方法，调用时不必到js中获取
  v8::Persistent<v8::Function> console_log;    // 缓存方法，调用时不必到js中获取
  v8::Persistent<v8::Object> request_context;  // 在一个请求生命周期内缓存的插件context对象
  v8::Persistent<v8::ObjectTemplate> request_context_templ;  // 插件context对象的构造模版
  v8::HeapStatistics hs;                                     // 保存v8堆栈采样信息的对象
  std::unordered_set<std::string> check_points;              // 所有检测点名称
  bool is_timeout = false;                                   // 超时标志
  bool is_oom = false;                                       // 内存满标志
  uint64_t timestamp = 0;                                    // 创建时间
  void* custom_data = nullptr;                               // php或java环境中额外非公共的数据
};

// v8::StartupData是一个构造好的js运行环境的快照
// Snapshot增加了数据保存和载入方法
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

// v8::Isolate是一个独立的js运行环境，使用时必须绑定到线程
// Isolate增加了一些工具方法，因为不能够增加对象，所以通过SetData，GetData来绑定获取数据
class Isolate : public v8::Isolate {
 public:
  static Isolate* New(Snapshot* snapshot_blob, uint64_t timestamp);
  Isolate() = delete;
  ~Isolate() = delete;
  void Initialize();
  IsolateData* GetData();
  void SetData(IsolateData* data);
  void Dispose();
  bool IsDead();
  bool IsExpired(uint64_t timestamp);
  // 废弃，使用返回v8::MaybeLocal的方法
  v8::Local<v8::Array> Check(v8::Local<v8::String> request_type,
                             v8::Local<v8::Object> request_params,
                             v8::Local<v8::Object> request_context,
                             int timeout = 100);
  v8::MaybeLocal<v8::Array> Check(v8::Local<v8::Context> context,
                                  v8::Local<v8::String> request_type,
                                  v8::Local<v8::Object> request_params,
                                  v8::Local<v8::Object> request_context,
                                  int timeout = 100);
  v8::MaybeLocal<v8::Value> ExecScript(const std::string& source, const std::string& filename, int line_offset = 0);
  v8::MaybeLocal<v8::Value> ExecScript(v8::Local<v8::String> source,
                                       v8::Local<v8::String> filename,
                                       v8::Local<v8::Integer> line_offset);
  v8::MaybeLocal<v8::Value> Log(v8::Local<v8::Value> value);
};

// 中断超时的js执行，任务在v8::Platform的后台线程池中执行
class TimeoutTask : public v8::Task {
 public:
  TimeoutTask(Isolate* isolate, std::future<void> fut, int milliseconds = 100);
  void Run() override;

 private:
  Isolate* isolate;
  std::future<void> fut;
  std::chrono::time_point<std::chrono::high_resolution_clock> time_point;
};

// 为异步request请求提供线程池和队列
class ThreadPool;
class HTTPRequest;
class AsyncRequest {
 public:
  AsyncRequest(std::shared_ptr<ThreadPool> pool);
  bool Submit(std::shared_ptr<HTTPRequest> request);
  size_t GetQueueSize();

  static void ConfigInstance(size_t pool_size, size_t queue_cap);
  static AsyncRequest& GetInstance();
  static void Terminate();

 private:
  std::shared_ptr<ThreadPool> pool;
  static size_t pool_size;
  static size_t queue_cap;
};

// v8整体（不是isolate）初始化过程
inline bool Initialize(size_t pool_size, Logger logger, size_t request_pool_size = 1, size_t request_queue_cap = 100) {
  // 这两个flag关闭新生代，只使用老生代，更省内存
  std::string flags = "--stress-compaction --stress-compaction-random ";
  const char* env = std::getenv("OPENRASP_V8_OPTIONS");
  if (env) {
    flags += env;
  }
  v8::V8::SetFlagsFromString(flags.data(), flags.size());
  Platform::logger = logger;
  v8::V8::InitializePlatform(Platform::New(pool_size));
  // v8::V8::SetDcheckErrorHandler([](const char* file, int line, const char* message) {
  //   std::string msg = "\nDebug check failed: ";
  //   msg += message;
  //   msg += ", in ";
  //   msg += file;
  //   msg += ", line ";
  //   msg += std::to_string(line);
  //   msg += "\n";
  //   Platform::logger(msg);
  //   printf("%s", msg.c_str());
  // });
  AsyncRequest::ConfigInstance(request_pool_size, request_queue_cap);
  return v8::V8::Initialize();
}

inline bool Dispose() {
  AsyncRequest::GetInstance().Terminate();
  bool rst = v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  return rst;
}

}  // namespace openrasp_v8

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