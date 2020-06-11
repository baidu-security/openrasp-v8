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

// 创建isolate对象
Isolate* Isolate::New(Snapshot* snapshot_blob, uint64_t timestamp) {
  static v8::ArrayBuffer::Allocator* array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  IsolateData* data = new IsolateData();
  data->create_params.array_buffer_allocator = array_buffer_allocator;
  data->create_params.snapshot_blob = snapshot_blob;
  data->create_params.external_references = snapshot_blob->external_references;  // native方法的指针
  // 设置新生代，老生代大小，目前已经不需要了
  // data->create_params.constraints.set_max_young_generation_size_in_bytes(1);
  // data->create_params.constraints.set_max_old_generation_size_in_bytes(20 * 1024 * 1024);
  // data->create_params.constraints.set_initial_old_generation_size_in_bytes(1024 * 1024 / 2);
  // data->create_params.constraints.set_max_semi_space_size_in_kb(1024);
  // data->create_params.constraints.set_max_old_space_size(20);

  Isolate* isolate = reinterpret_cast<Isolate*>(v8::Isolate::New(data->create_params));
  if (!isolate) {
    return nullptr;
  }
  isolate->SetFatalErrorHandler([](const char* location, const char* message) {
    std::string msg;
    msg += "\n#\n# Native error in ";
    msg += location;
    msg += "\n# ";
    msg += message;
    msg += "\n#\n\n";
    Platform::logger(msg);
    printf("%s", msg.c_str());
  });
  // 每次GC后对isolate的堆采样，超出20M终止执行
  isolate->AddGCEpilogueCallback(
      [](v8::Isolate* isolate, v8::GCType type, v8::GCCallbackFlags flags, void* d) {
        auto data = reinterpret_cast<IsolateData*>(d);
        isolate->GetHeapStatistics(&data->hs);
        if (data->hs.used_heap_size() > 20 * 1024 * 1024) {
          Platform::logger("Javascript plugin execution out of memory\n");
          isolate->TerminateExecution();
          data->is_oom = true;
        }
      },
      data);
  // 这个回调通知堆栈快到上限了，马上就要崩了，返回比当前上限更大的值可临时增大上限
  isolate->AddNearHeapLimitCallback(
      [](void* data, size_t current_heap_limit, size_t initial_heap_limit) -> size_t {
        Platform::logger("Near v8 isolate heap limit\n");
        auto isolate = reinterpret_cast<Isolate*>(data);
        isolate->TerminateExecution();
        isolate->GetData()->is_oom = true;
        return current_heap_limit * 2;
      },
      isolate);
  isolate->SetData(data);
  data->timestamp = timestamp;

  return isolate;
}

// 初始化isolate对象
void Isolate::Initialize() {
  v8::HandleScope handle_scope(this);
  v8::Local<v8::Context> context = v8::Context::New(this);
  v8::Context::Scope context_scope(context);
  auto data = GetData();

  auto RASP = context->Global()->Get(context, NewV8Key(this, "RASP")).ToLocalChecked().As<v8::Object>();
  auto check = RASP->Get(context, NewV8Key(this, "check")).ToLocalChecked().As<v8::Function>();
  auto console_log = context->Global()
                         ->Get(context, NewV8Key(this, "console"))
                         .ToLocalChecked()
                         .As<v8::Object>()
                         ->Get(context, NewV8Key(this, "log"))
                         .ToLocalChecked()
                         .As<v8::Function>();
  auto check_points = RASP->Get(context, NewV8Key(this, "checkPoints"))
                          .ToLocalChecked()
                          .As<v8::Object>()
                          ->GetOwnPropertyNames(context)
                          .ToLocalChecked();
  for (uint32_t i = 0; i < check_points->Length(); i++) {
    auto key = check_points->Get(context, i).ToLocalChecked();
    v8::String::Utf8Value val(this, key);
    data->check_points.emplace(*val, val.length());
  }
  data->context.Reset(this, context);
  data->RASP.Reset(this, RASP);
  data->check.Reset(this, check);
  data->console_log.Reset(this, console_log);
}

IsolateData* Isolate::GetData() {
  return reinterpret_cast<IsolateData*>(v8::Isolate::GetData(0));
}

void Isolate::SetData(IsolateData* data) {
  v8::Isolate::SetData(0, data);
}

void Isolate::Dispose() {
  delete GetData();
  v8::Isolate::Dispose();
}

bool Isolate::IsDead() {
  return v8::Isolate::IsDead() || GetData()->is_oom;
}

bool Isolate::IsExpired(uint64_t timestamp) {
  return timestamp > GetData()->timestamp;
}

// 执行检测
v8::MaybeLocal<v8::Array> Isolate::Check(v8::Local<v8::Context> context,
                                         v8::Local<v8::String> request_type,
                                         v8::Local<v8::Object> request_params,
                                         v8::Local<v8::Object> request_context,
                                         int timeout) {
  auto isolate = this;
  v8::EscapableHandleScope handle_scope(isolate);
  auto data = isolate->GetData();
  v8::TryCatch try_catch(isolate);
  auto check = data->check.Get(isolate);
  v8::Local<v8::Value> argv[]{request_type, request_params, request_context};

  // 提交后台超时任务
  std::promise<void> pro;
  Platform::Get()->CallOnWorkerThread(std::unique_ptr<v8::Task>(new TimeoutTask(isolate, pro.get_future(), timeout)));
  auto maybe_rst = check->Call(context, check, 3, argv);
  pro.set_value();
  // 必须pump剩余任务
  while (Platform::Get()->PumpMessageLoop(isolate)) {
    continue;
  }

  if (UNLIKELY(maybe_rst.IsEmpty())) {
    if (data->is_timeout) {
      data->is_timeout = false;
      Platform::logger("Javascript plugin execution timeout\n");
    }
    if (try_catch.HasTerminated()) {
      isolate->CancelTerminateExecution();
    } else {
      Platform::logger(Exception(isolate, try_catch));
    }
    return {};
  }
  auto rst = maybe_rst.ToLocalChecked();
  if (UNLIKELY(!rst->IsArray())) {
    return {};
  }
  auto arr = rst.As<v8::Array>();
  // all results are ignore, fast track
  if (LIKELY(arr->Length() == 0)) {
    return handle_scope.Escape(arr);
  }
  // any result is promise or any result.action is not equal to ignore
  // we do not care abort the performace here
  v8::Local<v8::Array> ret_arr = v8::Array::New(isolate);
  int idx = 0;
  for (int i = 0; i < arr->Length(); i++) {
    v8::Local<v8::Value> item;
    if (arr->Get(context, i).ToLocal(&item)) {
      if (item->IsPromise()) {
        item = item.As<v8::Promise>()->Result();
        if (item->IsUndefined()) {
          continue;
        }
      }
      ret_arr->Set(context, idx++, item).IsJust();
    }
  }
  return handle_scope.Escape(ret_arr);
}

v8::Local<v8::Array> Isolate::Check(v8::Local<v8::String> request_type,
                                    v8::Local<v8::Object> request_params,
                                    v8::Local<v8::Object> request_context,
                                    int timeout) {
  auto maybe_rst = Check(this->GetCurrentContext(), request_type, request_params, request_context, timeout);
  if (maybe_rst.IsEmpty()) {
    return v8::Array::New(this, 0);
  } else {
    return maybe_rst.ToLocalChecked();
  }
}

// 执行任意js代码
v8::MaybeLocal<v8::Value> Isolate::ExecScript(const std::string& source, const std::string& filename, int line_offset) {
  auto isolate = this;
  v8::EscapableHandleScope handle_scope(isolate);
  return handle_scope.EscapeMaybe(
      ExecScript(NewV8String(isolate, source), NewV8String(isolate, filename), v8::Integer::New(isolate, line_offset)));
}

v8::MaybeLocal<v8::Value> Isolate::ExecScript(v8::Local<v8::String> source,
                                              v8::Local<v8::String> filename,
                                              v8::Local<v8::Integer> line_offset) {
  auto isolate = this;
  v8::EscapableHandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  v8::ScriptOrigin origin(filename, line_offset);
  v8::Local<v8::Script> script;
  if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
    return handle_scope.EscapeMaybe(v8::MaybeLocal<v8::Value>());
  }
  auto rst = script->Run(context);
  while (Platform::Get()->PumpMessageLoop(isolate)) {
    continue;
  }
  return handle_scope.EscapeMaybe(rst);
}

v8::MaybeLocal<v8::Value> Isolate::Log(v8::Local<v8::Value> value) {
  auto isolate = this;
  v8::EscapableHandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  auto console_log = isolate->GetData()->console_log.Get(isolate);
  auto rst = console_log->Call(context, console_log, 1, &value);
  return handle_scope.EscapeMaybe(rst);
}

}  // namespace openrasp_v8