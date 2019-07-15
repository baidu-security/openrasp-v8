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
#include <iostream>
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

openrasp::Isolate* GetIsolate(JNIEnv* env);
std::string Jstring2String(JNIEnv* env, jstring jstr);
jstring String2Jstring(JNIEnv* env, const std::string& str);
v8::MaybeLocal<v8::String> Jstring2V8string(JNIEnv* env, jstring jstr);
jstring V8value2Jstring(JNIEnv* env, v8::Local<v8::Value> val);
v8::Local<v8::ObjectTemplate> CreateRequestContextTemplate(openrasp::Isolate* isolate);

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

inline JNIEnv* GetJNIEnv(openrasp::Isolate* isolate) {
  return reinterpret_cast<JNIEnv*>(isolate->GetData()->custom_data);
}

class IsolateDeleter {
 public:
  ALIGN_FUNCTION void operator()(openrasp::Isolate* isolate) {
    if (isolate) {
      isolate->Dispose();
    }
  }
};
typedef std::unique_ptr<openrasp::Isolate, IsolateDeleter> IsolatePtr;

extern V8Class v8_class;
extern ContextClass ctx_class;
extern bool isInitialized;
extern openrasp::Snapshot* snapshot;
extern std::mutex mtx;
