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

using namespace openrasp_v8;

JavaVM* jvm = nullptr;
V8Class v8_class;
ContextClass ctx_class;
bool is_initialized = false;
Snapshot* snapshot = nullptr;
std::mutex snapshot_mtx;
thread_local PerThreadRuntime per_thread_runtime;

void plugin_log(JNIEnv* env, const std::string& message) {
  auto msg = String2Jstring(env, message);
  env->CallStaticVoidMethod(v8_class.cls, v8_class.Log, msg);
}

void plugin_log(const std::string& message) {
  JNIEnv* env;
  auto rst = jvm->GetEnv((void**)&env, JNI_VERSION_1_6);
  if (rst == JNI_ERR) {
    printf("%s", message.c_str());
    return;
  }
  if (rst == JNI_EDETACHED) {
    jvm->AttachCurrentThread((void**)&env, nullptr);
  }
  plugin_log(env, message);
  if (rst == JNI_EDETACHED) {
    jvm->DetachCurrentThread();
  }
}

void GetStack(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto isolate = reinterpret_cast<openrasp_v8::Isolate*>(info.GetIsolate());
  auto env = GetJNIEnv(isolate);
  jbyteArray jbuf = reinterpret_cast<jbyteArray>(env->CallStaticObjectMethod(v8_class.cls, v8_class.GetStack));
  if (jbuf == nullptr) {
    return info.GetReturnValue().Set(v8::Array::New(isolate));
  }
  auto maybe_string = v8::String::NewExternalOneByte(isolate, new ExternalOneByteStringResource(env, jbuf));
  if (maybe_string.IsEmpty()) {
    return info.GetReturnValue().Set(v8::Array::New(isolate));
  }
  auto maybe_value = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
  if (maybe_value.IsEmpty()) {
    return info.GetReturnValue().Set(v8::Array::New(isolate));
  }
  auto value = maybe_value.ToLocalChecked();
  info.GetReturnValue().Set(value);
}

std::string Jstring2String(JNIEnv* env, jstring str) {
  try {
    auto size = env->GetStringLength(str);
    if (size < 0) {
      return {};
    }
    if (size > max_buffer_size) {
      size = max_buffer_size;
    }
    std::vector<char16_t> u16(size);
    env->GetStringRegion(str, 0, size, reinterpret_cast<jchar*>(u16.data()));
#ifdef _WIN32
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(
        std::wstring(u16.begin(), u16.end()));
#else
    return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.to_bytes(u16.data(), u16.data() + size);
#endif
  } catch (std::exception& exception) {
    std::string s;
    auto utf = env->GetStringUTFChars(str, nullptr);
    auto size = env->GetStringUTFLength(str);
    if (utf && size > 0) {
      s = std::string(utf, size < max_buffer_size ? size : max_buffer_size);
    }
    env->ReleaseStringUTFChars(str, utf);
    return s;
  }
}

jstring String2Jstring(JNIEnv* env, const std::string& str) {
  try {
#ifdef _WIN32
    std::wstring ws = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(str);
    std::u16string u16(ws.begin(), ws.end());
#else
    std::u16string u16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(str);
#endif
    return env->NewString(reinterpret_cast<const jchar*>(u16.data()), u16.size());
  } catch (std::exception& exception) {
    return env->NewStringUTF(str.data());
  }
}

v8::MaybeLocal<v8::String> Jstring2V8string(JNIEnv* env, jstring jstr) {
  auto size = env->GetStringLength(jstr);
  if (size < 0) {
    return {};
  }
  if (size > max_buffer_size) {
    size = max_buffer_size;
  }
  std::vector<jchar> u16(size);
  env->GetStringRegion(jstr, 0, size, u16.data());
  return v8::String::NewFromTwoByte(v8::Isolate::GetCurrent(), u16.data(), v8::NewStringType::kNormal, size);
}

jstring V8value2Jstring(JNIEnv* env, v8::Local<v8::Value> val) {
  v8::String::Value str(v8::Isolate::GetCurrent(), val);
  return env->NewString(*str, str.length());
}