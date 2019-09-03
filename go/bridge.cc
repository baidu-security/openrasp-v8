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

using namespace openrasp_v8;

Snapshot* snapshot = nullptr;
std::vector<PluginFile> plugin_list;

char Initialize() {
  v8::V8::InitializePlatform(Platform::New(0));
  v8::V8::Initialize();
  return true;
}

char Dispose() {
  delete snapshot;
  v8::V8::Dispose();
  v8::V8::ShutdownPlatform();
  return true;
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
    return false;
  }
  delete snapshot;
  snapshot = blob;
  return true;
}

Buffer Check(Buffer type, Buffer params, int context_index, int timeout) {
  Isolate* isolate = GetIsolate();
  if (!isolate) {
    return {nullptr, 0};
  }
  auto data = isolate->GetData();
  auto custom_data = GetCustomData(data);
  custom_data->context_index = context_index;
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::String> tmp_str;
  v8::Local<v8::String> request_type;
  v8::Local<v8::Value> request_params;
  v8::Local<v8::Object> request_context;

  if (!v8::String::NewFromUtf8(isolate, *type, v8::NewStringType::kNormal, type.length()).ToLocal(&request_type)) {
    return {nullptr, 0};
  }

  if (!v8::String::NewFromUtf8(isolate, *params, v8::NewStringType::kNormal, params.length()).ToLocal(&tmp_str)) {
    return {nullptr, 0};
  }
  if (!v8::JSON::Parse(context, tmp_str).ToLocal(&request_params)) {
    return {nullptr, 0};
  }

  if (!data->request_context_templ.Get(isolate)->NewInstance(context).ToLocal(&request_context)) {
    return {nullptr, 0};
  }

  auto rst = isolate->Check(request_type, request_params.As<v8::Object>(), request_context, timeout);

  if (rst->Length() == 0) {
    return {nullptr, 0};
  }
  v8::Local<v8::String> json;
  if (!v8::JSON::Stringify(context, rst).ToLocal(&json)) {
    return {nullptr, 0};
  }
  size_t len = json->Utf8Length(isolate);
  // no trailing 0
  char* str = new char[len];
  json->WriteUtf8(isolate, str, len);
  return {str, len};
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
    plugin_info(isolate, e);
    return {nullptr, 0};
  }
  size_t len = string->Utf8Length(isolate);
  // no trailing 0
  char* str = new char[len];
  string->WriteUtf8(isolate, str, len);
  return {str, len};
}
