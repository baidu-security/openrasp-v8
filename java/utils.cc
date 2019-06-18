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
#include <codecvt>
#include <locale>
#include <vector>
#include "header.h"

using namespace openrasp;

V8Class v8_class;
ContextClass ctx_class;
bool isInitialized = false;
Snapshot* snapshot = nullptr;
std::mutex mtx;

void openrasp::plugin_info(Isolate* isolate, const std::string& message) {
  auto data = isolate->GetData();
  auto custom_data = GetCustomData(isolate);
  if (!custom_data || !custom_data->env) {
    return;
  }
  auto env = custom_data->env;
  auto msg = String2Jstring(env, message);
  env->CallStaticVoidMethod(v8_class.cls, v8_class.plugin_log, msg);
}

Isolate* GetIsolate() {
  static thread_local IsolatePtr isolate_ptr;
  Isolate* isolate = isolate_ptr.get();
  if (snapshot) {
    if (!isolate || isolate->IsExpired(snapshot->timestamp)) {
      std::unique_lock<std::mutex> lock = isolate ? std::unique_lock<std::mutex>(mtx, std::try_to_lock)
                                                  : std::unique_lock<std::mutex>(mtx);
      if (lock) {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        isolate = Isolate::New(snapshot, millis);
        isolate->GetData()->custom_data = new CustomData();
        isolate_ptr.reset(isolate);
      }
    }
  }
  return isolate;
}

std::string Jstring2String(JNIEnv* env, jstring str) {
  auto data = env->GetStringCritical(str, nullptr);
  auto size = env->GetStringLength(str);
  // 在windows上用u16string会报错，只能这样拷贝转换
  std::wstring u16(size, 0);
  for (int i = 0; i < size; i++) {
    u16[i] = data[i];
  }
  env->ReleaseStringCritical(str, data);
  return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(u16);
}

jstring String2Jstring(JNIEnv* env, const std::string& str) {
  std::wstring u16 = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(str);
  auto size = u16.size();
  std::vector<jchar> data(size, 0);
  for (int i = 0; i < size; i++) {
    data[i] = u16[i];
  }
  return env->NewString(data.data(), size);
}

v8::MaybeLocal<v8::String> Jstring2V8string(JNIEnv* env, jstring jstr) {
  auto data = env->GetStringCritical(jstr, nullptr);
  auto size = env->GetStringLength(jstr);
  auto rst =
      v8::String::NewFromTwoByte(GetIsolate(), static_cast<const uint16_t*>(data), v8::NewStringType::kNormal, size);
  env->ReleaseStringCritical(jstr, data);
  return rst;
}

jstring V8value2Jstring(JNIEnv* env, v8::Local<v8::Value> val) {
  v8::String::Value str(GetIsolate(), val);
  return env->NewString(*str, str.length());
}