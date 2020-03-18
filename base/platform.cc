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

#include "bundle.h"

namespace openrasp_v8 {

std::unique_ptr<Platform> Platform::instance;
Logger Platform::logger = [](const std::string& message) { printf("%s", message.c_str()); };
// CriticalMemoryPressureCallback Platform::criticalMemoryPressureCallback = nullptr;

Platform* Platform::New(int thread_pool_size) {
  instance.reset(new Platform(thread_pool_size));
  return instance.get();
}

Platform* Platform::Get() {
  return instance.get();
}

void Platform::Startup() {
  if (!default_platform) {
    default_platform = v8::platform::NewDefaultPlatform(thread_pool_size);
  }
}

void Platform::Shutdown() {
  if (default_platform) {
    default_platform = nullptr;
  }
}

Platform::Platform(int thread_pool_size) : thread_pool_size(thread_pool_size) {
  tracing_controller.reset(new v8::platform::tracing::TracingController());
  tracing_controller->Initialize(nullptr);
  Startup();
}

Platform::~Platform() {
  Shutdown();
}

bool Platform::PumpMessageLoop(v8::Isolate* isolate, v8::platform::MessageLoopBehavior behavior) {
  return v8::platform::PumpMessageLoop(default_platform.get(), isolate, behavior);
}
// void Platform::RunIdleTasks(v8::Isolate* isolate, double idle_time_in_seconds) {
//   return v8::platform::RunIdleTasks(default_platform.get(), isolate, idle_time_in_seconds);
// }
int Platform::NumberOfWorkerThreads() {
  return default_platform->NumberOfWorkerThreads();
}
std::shared_ptr<v8::TaskRunner> Platform::GetForegroundTaskRunner(v8::Isolate* isolate) {
  return default_platform->GetForegroundTaskRunner(isolate);
}
void Platform::CallOnWorkerThread(std::unique_ptr<v8::Task> task) {
  return default_platform->CallOnWorkerThread(std::move(task));
}
void Platform::CallDelayedOnWorkerThread(std::unique_ptr<v8::Task> task, double delay_in_seconds) {
  return default_platform->CallDelayedOnWorkerThread(std::move(task), delay_in_seconds);
}
void Platform::CallOnForegroundThread(v8::Isolate* isolate, v8::Task* task) {
  return default_platform->CallOnForegroundThread(isolate, task);
}
void Platform::CallDelayedOnForegroundThread(v8::Isolate* isolate, v8::Task* task, double delay_in_seconds) {
  return default_platform->CallDelayedOnForegroundThread(isolate, task, delay_in_seconds);
}
// void Platform::CallIdleOnForegroundThread(v8::Isolate* isolate, v8::IdleTask* task) {
//   return default_platform->CallIdleOnForegroundThread(isolate, task);
// }
// bool Platform::IdleTasksEnabled(v8::Isolate* isolate) {
//   return default_platform->IdleTasksEnabled(isolate);
// }
double Platform::MonotonicallyIncreasingTime() {
  return default_platform->MonotonicallyIncreasingTime();
}
double Platform::CurrentClockTimeMillis() {
  return default_platform->CurrentClockTimeMillis();
}
v8::TracingController* Platform::GetTracingController() {
  // return default_platform->GetTracingController();
  // return our TracingController, to avoid nullptr exception when shutdown platform
  return tracing_controller.get();
}
v8::Platform::StackTracePrinter Platform::GetStackTracePrinter() {
  return default_platform->GetStackTracePrinter();
}
// v8::PageAllocator* Platform::GetPageAllocator() {
//   // return default_platform->GetPageAllocator();
//   // if returned nullptr, v8 will create a default PageAllocator as same as the one of default platform
//   return nullptr;
// }
// bool Platform::OnCriticalMemoryPressure(size_t length) {
//   if (criticalMemoryPressureCallback != nullptr) {
//     return criticalMemoryPressureCallback(length);
//   } else {
//     return false;
//   }
// }

}  // namespace openrasp_v8