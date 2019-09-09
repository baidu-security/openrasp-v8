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
#include <queue>
#include <thread>
#include "base/bundle.h"

// fix 32 bit jdk's runtime stack
#if defined(__linux__) && defined(__i386__)
#define ALIGN_FUNCTION __attribute__((force_align_arg_pointer))
#else
#define ALIGN_FUNCTION
#endif

extern openrasp_v8::Snapshot* snapshot;
extern std::mutex mtx;

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
  }
  jclass cls;
  jmethodID Log;
  jmethodID GetStack;
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

class IsolateDeleter {
 public:
  ALIGN_FUNCTION void operator()(openrasp_v8::Isolate* isolate) { isolate->Dispose(); }
};

class PerThreadRuntime {
 public:
  ~PerThreadRuntime() {
    std::lock_guard<std::mutex> lock(shared_isolate_mtx);
    if (isolate) {
      shared_isolate.emplace(isolate);
    }
    Dispose();
  }
  openrasp_v8::Isolate* GetIsolate() {
    if (!isolate || (snapshot && isolate->IsExpired(snapshot->timestamp))) {
      std::lock_guard<std::mutex> lock_shared_isolate(shared_isolate_mtx);
      Dispose();
      while (!shared_isolate.empty()) {
        isolate = shared_isolate.front().lock();
        shared_isolate.pop();
        if (!isolate || (snapshot && isolate->IsExpired(snapshot->timestamp))) {
          isolate.reset();
        } else {
          break;
        }
      }
      std::lock_guard<std::mutex> lock_snapshot(mtx);
      if (!isolate && snapshot) {
        auto duration = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        isolate.reset(openrasp_v8::Isolate::New(snapshot, millis), IsolateDeleter());
        shared_isolate.emplace(isolate);
        v8::Locker lock(isolate.get());
        v8::Isolate::Scope isolate_scope(isolate.get());
        v8::HandleScope handle_scope(isolate.get());
        isolate->Initialize();
        isolate->GetData()->request_context_templ.Reset(isolate.get(), CreateRequestContextTemplate(isolate.get()));
      }
    }
    return isolate.get();
  }
  void Dispose() {
    request_context.Reset();
    isolate.reset();
  }
  static std::queue<std::weak_ptr<openrasp_v8::Isolate>> shared_isolate;
  static std::mutex shared_isolate_mtx;
  std::shared_ptr<openrasp_v8::Isolate> isolate;
  v8::Persistent<v8::Object> request_context;
};

extern JavaVM* jvm;
extern V8Class v8_class;
extern ContextClass ctx_class;
extern bool is_initialized;
extern thread_local PerThreadRuntime per_thread_runtime;
