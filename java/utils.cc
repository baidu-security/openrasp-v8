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
  auto env = GetJNIEnv(isolate);
  auto msg = String2Jstring(env, message);
  env->CallStaticVoidMethod(v8_class.cls, v8_class.Log, msg);
}

v8::Local<v8::Array> openrasp::get_stack(Isolate* isolate) {
  auto env = GetJNIEnv(isolate);
  jbyteArray jbuf = reinterpret_cast<jbyteArray>(env->CallStaticObjectMethod(v8_class.cls, v8_class.GetStack));
  if (jbuf == nullptr) {
    return v8::Array::New(isolate, 0);
  }
  auto raw = env->GetPrimitiveArrayCritical(jbuf, nullptr);
  auto maybe_string = v8::String::NewFromOneByte(isolate, static_cast<uint8_t*>(raw), v8::NewStringType::kNormal);
  env->ReleasePrimitiveArrayCritical(jbuf, raw, JNI_ABORT);
  if (maybe_string.IsEmpty()) {
    return v8::Array::New(isolate, 0);
  }
  auto maybe_value = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
  if (maybe_value.IsEmpty()) {
    return v8::Array::New(isolate, 0);
  }
  auto value = maybe_value.ToLocalChecked();
  if (!value->IsArray()) {
    return v8::Array::New(isolate, 0);
  }
  return value.As<v8::Array>();
}

Isolate* GetIsolate(JNIEnv* env) {
  static thread_local IsolatePtr isolate_ptr;
  Isolate* isolate = isolate_ptr.get();
  if (snapshot) {
    if (!isolate || isolate->IsExpired(snapshot->timestamp)) {
      std::unique_lock<std::mutex> lock =
          isolate ? std::unique_lock<std::mutex>(mtx, std::try_to_lock) : std::unique_lock<std::mutex>(mtx);
      if (lock) {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        isolate = Isolate::New(snapshot, millis);
        isolate_ptr.reset(isolate);
        v8::HandleScope handle_scope(isolate);
        isolate->GetData()->request_context_templ.Reset(isolate, CreateRequestContextTemplate(env));
      }
    }
  }
  if (isolate) {
    isolate->GetData()->custom_data = env;
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
      v8::String::NewFromTwoByte(GetIsolate(env), static_cast<const uint16_t*>(data), v8::NewStringType::kNormal, size);
  env->ReleaseStringCritical(jstr, data);
  return rst;
}

jstring V8value2Jstring(JNIEnv* env, v8::Local<v8::Value> val) {
  v8::String::Value str(GetIsolate(env), val);
  return env->NewString(*str, str.length());
}