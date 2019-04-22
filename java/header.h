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

#include <chrono>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include "base/bundle.h"
#include "com_baidu_openrasp_plugin_v8_V8.h"

class V8Class {
 public:
  V8Class() = default;
  V8Class(JNIEnv* env) {
    cls = env->FindClass("com/baidu/openrasp/plugin/v8/V8");
    plugin_log = env->GetStaticMethodID(cls, "PluginLog", "(Ljava/lang/String;)V");
  }
  jclass cls;
  jmethodID plugin_log;
};

class ContextClass {
 public:
  ContextClass() = default;
  ContextClass(JNIEnv* env) {
    cls = env->FindClass("com/baidu/openrasp/plugin/v8/Context");
    getPath = env->GetMethodID(cls, "getPath", "()Ljava/lang/String;");
    getMethod = env->GetMethodID(cls, "getMethod", "()Ljava/lang/String;");
    getUrl = env->GetMethodID(cls, "getUrl", "()Ljava/lang/String;");
    getQuerystring = env->GetMethodID(cls, "getQuerystring", "()Ljava/lang/String;");
    getAppBasePath = env->GetMethodID(cls, "getAppBasePath", "()Ljava/lang/String;");
    getProtocol = env->GetMethodID(cls, "getProtocol", "()Ljava/lang/String;");
    getRemoteAddr = env->GetMethodID(cls, "getRemoteAddr", "()Ljava/lang/String;");
    getJson = env->GetMethodID(cls, "getJson", "()Ljava/lang/String;");
    getBody = env->GetMethodID(cls, "getBody", "([I)[B");
    getHeader = env->GetMethodID(cls, "getHeader", "([I)[B");
    getParameter = env->GetMethodID(cls, "getParameter", "([I)[B");
    getServer = env->GetMethodID(cls, "getServer", "([I)[B");
  }
  jclass cls;
  jmethodID getPath;
  jmethodID getMethod;
  jmethodID getUrl;
  jmethodID getQuerystring;
  jmethodID getAppBasePath;
  jmethodID getProtocol;
  jmethodID getRemoteAddr;
  jmethodID getJson;
  jmethodID getBody;
  jmethodID getHeader;
  jmethodID getParameter;
  jmethodID getServer;
};

class CustomData {
 public:
  JNIEnv* env = nullptr;
  jobject context = nullptr;
};

inline CustomData* GetCustomData(openrasp::Isolate* isolate) {
  return reinterpret_cast<CustomData*>(isolate->GetData()->custom_data);
}

class IsolateDeleter {
 public:
  void operator()(openrasp::Isolate* isolate) {
    delete GetCustomData(isolate);
    isolate->Dispose();
  }
};
typedef std::unique_ptr<openrasp::Isolate, IsolateDeleter> IsolatePtr;

extern JavaVM* jvm;
extern V8Class v8_class;
extern ContextClass ctx_class;
extern bool isInitialized;
extern openrasp::Snapshot* snapshot;
extern std::mutex mtx;

openrasp::Isolate* GetIsolate();