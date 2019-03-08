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

#include "header.h"

using namespace openrasp;

bool is_initialized = false;
Snapshot* snapshot = nullptr;
std::mutex mtx;
std::vector<PluginFile> plugin_list;

void CreateV8String(void* isolate, void* maybe, Buffer buf) {
  *reinterpret_cast<v8::MaybeLocal<v8::String>*>(maybe) =
      v8::String::NewFromUtf8(reinterpret_cast<Isolate*>(isolate), *buf, v8::NewStringType::kNormal, buf.length());
}

void CreateV8ArrayBuffer(void* isolate, void* maybe, Buffer buf) {
  auto ab = v8::ArrayBuffer::New(reinterpret_cast<Isolate*>(isolate), buf.raw_size);
  auto contents = ab->GetContents();
  memcpy(contents.Data(), buf.data, buf.raw_size);
  *reinterpret_cast<v8::MaybeLocal<v8::ArrayBuffer>*>(maybe) = v8::MaybeLocal<v8::ArrayBuffer>(ab);
}

void* GetContextGetters(void* i) {
  auto isolate = reinterpret_cast<Isolate*>(i);
  auto custom_data = GetCustomData(isolate);
  return custom_data->context_getters;
}

char Initialize() {
  if (!is_initialized) {
    Platform::Initialize();
    v8::V8::Initialize();
    is_initialized = true;
  }
  return is_initialized;
}

char Dispose() {
  if (is_initialized) {
    delete snapshot;
    Platform::Shutdown();
    v8::V8::Dispose();
    is_initialized = false;
  }
  return !is_initialized;
}

char ClearPlugin() {
  plugin_list.clear();
  return true;
}

char AddPlugin(Buffer source, Buffer name) {
  plugin_list.emplace_back(std::string{*name, name.length()}, std::string{*source, source.length()});
  return true;
}

char CreateSnapshot(Buffer config) {
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  Snapshot* blob = new Snapshot({*config, config.length()}, plugin_list, millis, nullptr);
  if (!blob->IsOk()) {
    delete blob;
  } else {
    std::lock_guard<std::mutex> lock(mtx);
    delete snapshot;
    snapshot = blob;
  }
  return true;
}

char Check(Buffer type, Buffer params, void* context_getters) {
  Isolate* isolate = GetIsolate();
  if (!isolate) {
    return false;
  }
  auto data = isolate->GetData();
  auto custom_data = GetCustomData(data);
  custom_data->context_getters = context_getters;
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> tmp_str;
  v8::Local<v8::String> request_type;
  v8::Local<v8::Value> request_params;
  v8::Local<v8::Value> request_context;

  if (!v8::String::NewFromUtf8(isolate, *type, v8::NewStringType::kNormal, type.length()).ToLocal(&request_type)) {
    return false;
  }

  if (!v8::String::NewFromUtf8(isolate, *params, v8::NewStringType::kNormal, params.length()).ToLocal(&tmp_str)) {
    return false;
  }
  if (!v8::JSON::Parse(isolate->GetCurrentContext(), tmp_str).ToLocal(&request_params)) {
    return false;
  }

  data->request_context.Reset(isolate, data->request_context_templ.Get(isolate)->NewInstance());

  return isolate->Check(request_type, request_params.As<v8::Object>());
}

Buffer ExecScript(Buffer source, Buffer name) {
  Isolate* isolate = GetIsolate();
  if (!isolate) {
    return {nullptr, 0};
  }
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  auto maybe_rst = isolate->ExecScript({*source, source.length()}, {*name, name.length()});
  v8::Local<v8::Value> rst;
  v8::Local<v8::String> string;
  if (!maybe_rst.ToLocal(&rst) || !v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocal(&string)) {
    Exception e(isolate, try_catch);
    return {nullptr, 0};
  }
  size_t len = string->Utf8Length(isolate);
  // no trailing 0
  char* str = new char[len]{0};
  string->WriteUtf8(isolate, str, len);
  return {str, len};
}
