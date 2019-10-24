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

#include <jni.h>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include "base/bundle.h"

// fix 32 bit jdk's runtime stack
#if defined(__linux__) && defined(__i386__)
#define ALIGN_FUNCTION __attribute__((force_align_arg_pointer))
#else
#define ALIGN_FUNCTION
#endif

extern openrasp_v8::Snapshot* snapshot;
extern std::mutex snapshot_mtx;

void plugin_log(JNIEnv* env, const std::string& message);
void plugin_log(const std::string& message);
std::string Jstring2String(JNIEnv* env, jstring jstr);
jstring String2Jstring(JNIEnv* env, const std::string& str);
v8::MaybeLocal<v8::String> Jstring2V8string(JNIEnv* env, jstring jstr);
jstring V8value2Jstring(JNIEnv* env, v8::Local<v8::Value> val);
v8::Local<v8::ObjectTemplate> CreateRequestContextTemplate(openrasp_v8::Isolate* isolate);
void GetStack(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info);

class V8Class {
 public:
  V8Class() = default;
  V8Class(JNIEnv* env) {
    auto ref = env->FindClass("com/baidu/openrasp/v8/V8");
    cls = (jclass)env->NewGlobalRef(ref);
    env->DeleteLocalRef(ref);
    Log = env->GetStaticMethodID(cls, "Log", "(Ljava/lang/String;)V");
    GetStack = env->GetStaticMethodID(cls, "GetStack", "()[B");
    GetFreeMemory = env->GetStaticMethodID(cls, "GetFreeMemory", "()J");
    Gc = env->GetStaticMethodID(cls, "Gc", "()V");
  }
  jclass cls;
  jmethodID Log;
  jmethodID GetStack;
  jmethodID GetFreeMemory;
  jmethodID Gc;
};

class ContextClass {
 public:
  ContextClass() = default;
  ContextClass(JNIEnv* env) {
    auto ref = env->FindClass("com/baidu/openrasp/v8/Context");
    cls = (jclass)env->NewGlobalRef(ref);
    env->DeleteLocalRef(ref);
    getString = env->GetMethodID(cls, "getString", "(Ljava/lang/String;)Ljava/lang/String;");
    getObject = env->GetMethodID(cls, "getObject", "(Ljava/lang/String;)[B");
    getBuffer = env->GetMethodID(cls, "getBuffer", "(Ljava/lang/String;)[B");
  }
  jclass cls;
  jmethodID getString;
  jmethodID getObject;
  jmethodID getBuffer;
};

inline JNIEnv* GetJNIEnv(openrasp_v8::Isolate* isolate) {
  return reinterpret_cast<JNIEnv*>(isolate->GetData()->custom_data);
}

namespace isolate_pool {
extern size_t size;
extern std::shared_ptr<openrasp_v8::Isolate> GetIsolate();
}  // namespace isolate_pool

class PerThreadRuntime {
 public:
  ~PerThreadRuntime() { Dispose(); }
  openrasp_v8::Isolate* GetIsolate() {
    if (!snapshot) {
      return nullptr;
    }
    if (!isolate || isolate->IsDead() || isolate->IsExpired(snapshot->timestamp)) {
      Dispose();
      isolate = isolate_pool::GetIsolate();
    }
    return isolate.get();
  }
  void Dispose() {
    request_context.Reset();
    isolate.reset();
  }
  std::shared_ptr<openrasp_v8::Isolate> isolate;
  v8::Persistent<v8::Object> request_context;
};

class ExternalOneByteStringResource : public v8::String::ExternalOneByteStringResource {
 public:
  ExternalOneByteStringResource(JNIEnv* env, jbyteArray jbuf, size_t jsize = 0) {
    if (jbuf == nullptr) {
      return;
    }
    if (jsize <= 0) {
      jsize = env->GetArrayLength(jbuf);
      if (jsize <= 0) {
        return;
      }
    }
    buf = new char[jsize];
    if (buf == nullptr) {
      return;
    }
    env->GetByteArrayRegion(jbuf, 0, jsize, reinterpret_cast<jbyte*>(buf));
    size = strnlen(buf, jsize);
  };
  ~ExternalOneByteStringResource() override { delete[] buf; }
  const char* data() const override { return buf; }
  size_t length() const override { return size; }

 private:
  size_t size = 0;
  char* buf = nullptr;
};

class ExternalStringResource : public v8::String::ExternalStringResource {
 public:
  ExternalStringResource(JNIEnv* env, jstring jstr) {
    if (jstr == nullptr) {
      return;
    }
    size = env->GetStringLength(jstr);
    if (size <= 0) {
      size = 0;
      return;
    }
    buf = new uint16_t[size];
    if (buf == nullptr) {
      return;
    }
    env->GetStringRegion(jstr, 0, size, reinterpret_cast<jchar*>(buf));
  };
  ~ExternalStringResource() override { delete[] buf; }
  const uint16_t* data() const override { return buf; }
  size_t length() const override { return size; }

 private:
  size_t size = 0;
  uint16_t* buf = nullptr;
};

extern JavaVM* jvm;
extern V8Class v8_class;
extern ContextClass ctx_class;
extern bool is_initialized;
extern thread_local PerThreadRuntime per_thread_runtime;
