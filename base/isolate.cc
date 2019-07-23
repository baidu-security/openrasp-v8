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

namespace openrasp {

Isolate* Isolate::New(Snapshot* snapshot_blob, uint64_t timestamp) {
  static v8::ArrayBuffer::Allocator* array_buffer_allocator = v8::ArrayBuffer::Allocator::NewDefaultAllocator();
  IsolateData* data = new IsolateData();
  data->create_params.array_buffer_allocator = array_buffer_allocator;
  data->create_params.snapshot_blob = snapshot_blob;
  data->create_params.external_references = snapshot_blob->external_references;
  data->create_params.constraints.ConfigureDefaults(0, 0);

  Isolate* isolate = reinterpret_cast<Isolate*>(v8::Isolate::New(data->create_params));
#define DEFAULT_STACK_SIZE_IN_KB 1024
  uintptr_t current_stack = reinterpret_cast<uintptr_t>(&current_stack);
  uintptr_t stack_limit = current_stack - (DEFAULT_STACK_SIZE_IN_KB * 1024 / sizeof(uintptr_t));
  stack_limit = stack_limit < current_stack ? stack_limit : sizeof(stack_limit);
  isolate->SetStackLimit(stack_limit);
#undef DEFAULT_STACK_SIZE_IN_KB
  isolate->Enter();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = v8::Context::New(isolate);
  context->Enter();

  auto RASP = context->Global()->Get(context, NewV8Key(isolate, "RASP")).ToLocalChecked().As<v8::Object>();
  auto check = RASP->Get(context, NewV8Key(isolate, "check")).ToLocalChecked().As<v8::Function>();
  auto console_log = context->Global()
                         ->Get(NewV8Key(isolate, "console"))
                         .As<v8::Object>()
                         ->Get(NewV8Key(isolate, "log"))
                         .As<v8::Function>();

  data->RASP.Reset(isolate, RASP);
  data->check.Reset(isolate, check);
  data->console_log.Reset(isolate, console_log);
  data->timestamp = timestamp;

  isolate->SetData(data);

  return isolate;
}

IsolateData* Isolate::GetData() {
  return reinterpret_cast<IsolateData*>(v8::Isolate::GetData(0));
}

void Isolate::SetData(IsolateData* data) {
  v8::Isolate::SetData(0, data);
}

void Isolate::Dispose() {
  delete GetData();
  {
    v8::HandleScope handle_scope(this);
    v8::Local<v8::Context> context = GetCurrentContext();
    context->Exit();
  }
  Exit();
  v8::Isolate::Dispose();
}

bool Isolate::IsExpired(uint64_t timestamp) {
  return timestamp > GetData()->timestamp;
}

v8::Local<v8::Array> Isolate::Check(v8::Local<v8::String> type,
                                    v8::Local<v8::Object> params,
                                    v8::Local<v8::Object> context,
                                    int timeout) {
  auto isolate = this;
  auto data = isolate->GetData();
  auto v8_context = isolate->GetCurrentContext();
  v8::TryCatch try_catch(isolate);
  auto check = data->check.Get(isolate);
  v8::Local<v8::Value> argv[]{type, params, context};

  std::promise<void> pro;
  Platform::Get()->CallOnWorkerThread(std::unique_ptr<v8::Task>(new TimeoutTask(isolate, pro.get_future(), timeout)));
  auto maybe_rst = check->Call(v8_context, check, 3, argv);
  pro.set_value();

  if (UNLIKELY(maybe_rst.IsEmpty())) {
    auto msg = v8::Object::New(isolate);
    msg->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "exception"));
    if (try_catch.HasTerminated()) {
      msg->Set(NewV8Key(isolate, "message"), NewV8String(isolate, "Javascript plugin execution timeout"));
    } else {
      msg->Set(NewV8Key(isolate, "message"), NewV8String(isolate, Exception(isolate, try_catch)));
    }
    v8::Local<v8::Value> argv[]{msg.As<v8::Value>()};
    return v8::Array::New(isolate, argv, 1);
  }
  auto rst = maybe_rst.ToLocalChecked();
  if (UNLIKELY(!rst->IsArray())) {
    return v8::Array::New(isolate, 0);
  }
  auto arr = rst.As<v8::Array>();
  // all results are ignore, fast track
  if (LIKELY(arr->Length() == 0)) {
    return arr;
  }
  // any result is promise or any result.action is not equal to ignore
  // we do not care abort the performace here
  v8::Local<v8::Array> ret_arr = v8::Array::New(isolate);
  int idx = 0;
  for (int i = 0; i < arr->Length(); i++) {
    v8::Local<v8::Value> item;
    if (arr->Get(v8_context, i).ToLocal(&item)) {
      if (item->IsPromise()) {
        item = item.As<v8::Promise>()->Result();
        if (item->IsUndefined()) {
          continue;
        }
      }
      ret_arr->Set(v8_context, idx++, item).FromJust();
    }
  }
  return ret_arr;
}

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
  return handle_scope.EscapeMaybe(script->Run(context));
}

v8::MaybeLocal<v8::Value> Isolate::Log(v8::Local<v8::Value> value) {
  auto isolate = this;
  v8::EscapableHandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  auto console_log = isolate->GetData()->console_log.Get(isolate);
  return handle_scope.EscapeMaybe(console_log->Call(context, console_log, 1, &value));
}

}  // namespace openrasp