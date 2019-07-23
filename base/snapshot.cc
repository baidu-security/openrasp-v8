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

#include <cstdio>
#include "bundle.h"
#include "flex/flex.h"
#include "gen/builtins.h"

namespace openrasp {

Snapshot::Snapshot(const char* data, size_t raw_size, uint64_t timestamp)
    : v8::StartupData({data, static_cast<int>(raw_size)}), timestamp(timestamp) {}
Snapshot::Snapshot(const std::string& path, uint64_t timestamp) : v8::StartupData({nullptr, 0}), timestamp(timestamp) {
  char* buffer = nullptr;
  size_t size;
  std::ifstream file(path, std::ios::in | std::ios::binary);
  if (file) {
    file.seekg(0, std::ios::end);
    size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size > 0) {
      buffer = new char[size];
      if (!file.read(buffer, size)) {
        delete[] buffer;
        return;
      }
    }
  }
  this->data = buffer;
  this->raw_size = size;
}
Snapshot::Snapshot(const std::string& config,
                   const std::vector<PluginFile>& plugin_list,
                   const std::string& version,
                   uint64_t timestamp,
                   void* custom_data)
    : v8::StartupData({nullptr, 0}), timestamp(timestamp) {
  IsolateData data;
  data.custom_data = custom_data;
  v8::SnapshotCreator creator(external_references);
  Isolate* isolate = reinterpret_cast<Isolate*>(creator.GetIsolate());
  isolate->SetData(&data);
#define DEFAULT_STACK_SIZE_IN_KB 1024
  uintptr_t current_stack = reinterpret_cast<uintptr_t>(&current_stack);
  uintptr_t stack_limit = current_stack - (DEFAULT_STACK_SIZE_IN_KB * 1024 / sizeof(uintptr_t));
  stack_limit = stack_limit < current_stack ? stack_limit : sizeof(stack_limit);
  isolate->SetStackLimit(stack_limit);
#undef DEFAULT_STACK_SIZE_IN_KB
  {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context = v8::Context::New(isolate);
    v8::Context::Scope context_scope(context);
    v8::TryCatch try_catch(isolate);
    v8::Local<v8::Object> global = context->Global();
    global->Set(NewV8Key(isolate, "version"), NewV8String(isolate, version));
    global->Set(NewV8Key(isolate, "global"), global);
    global->Set(NewV8Key(isolate, "window"), global);
    v8::Local<v8::Object> v8_stdout = v8::Object::New(isolate);
    v8_stdout->Set(
        NewV8Key(isolate, "write"),
        v8::Function::New(context, reinterpret_cast<v8::FunctionCallback>(external_references[0])).ToLocalChecked());
    global->Set(NewV8Key(isolate, "stdout"), v8_stdout);
    global->Set(NewV8Key(isolate, "stderr"), v8_stdout);
    global->Set(
        NewV8Key(isolate, "flex_tokenize"),
        v8::Function::New(context, reinterpret_cast<v8::FunctionCallback>(external_references[1])).ToLocalChecked());
    global->Set(
        NewV8Key(isolate, "request"),
        v8::Function::New(context, reinterpret_cast<v8::FunctionCallback>(external_references[2])).ToLocalChecked());
    if (isolate->ExecScript({reinterpret_cast<const char*>(gen_builtins), gen_builtins_len}, "builtins.js").IsEmpty()) {
      Exception e(isolate, try_catch);
      plugin_info(isolate, e);
      // no need to continue
      return;
    }
    if (isolate->ExecScript(config, "config.js").IsEmpty()) {
      Exception e(isolate, try_catch);
      plugin_info(isolate, e);
    }
    for (auto& plugin_src : plugin_list) {
      if (isolate->ExecScript("(function(){\n" + plugin_src.source + "\n})()", plugin_src.filename, -1).IsEmpty()) {
        Exception e(isolate, try_catch);
        plugin_info(isolate, e);
      }
    }
    creator.SetDefaultContext(context);
  }
  v8::StartupData snapshot = creator.CreateBlob(v8::SnapshotCreator::FunctionCodeHandling::kClear);
  this->data = snapshot.data;
  this->raw_size = snapshot.raw_size;
}
Snapshot::~Snapshot() {
  delete[] data;
}
bool Snapshot::Save(const std::string& path) const {
  std::string tmp_path = path + ".tmp";
  std::ofstream file(tmp_path, std::ios::out | std::ios::binary | std::ios::trunc);
  if (file) {
    file.write(data, raw_size);
    file.close();
    if (!static_cast<bool>(file)) {
      return false;
    }
    if (std::rename(tmp_path.c_str(), path.c_str())) {
      return false;
    }
    return true;
  }
  // check errno when return value is false
  return false;
}
}  // namespace openrasp