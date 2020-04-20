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

#include "com_baidu_openrasp_v8_V8.h"

#include "header.h"

using namespace openrasp_v8;

ALIGN_FUNCTION JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
  jvm = vm;
  return JNI_VERSION_1_6;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Initialize
 * Signature: (III)Z
 */
ALIGN_FUNCTION JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_Initialize(JNIEnv* env,
                                                                                   jclass cls,
                                                                                   jint isolate_pool_size,
                                                                                   jint request_pool_size,
                                                                                   jint request_queue_size) {
  if (!is_initialized) {
    isolate_pool::size = isolate_pool_size;
    v8_class = V8Class(env);
    ctx_class = ContextClass(env);
    is_initialized = Initialize(0, plugin_log, request_pool_size, request_queue_size);
  }
  return is_initialized;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Dispose
 * Signature: ()Z
 */
ALIGN_FUNCTION JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_Dispose(JNIEnv* env, jclass cls) {
  if (is_initialized) {
    delete snapshot;
    is_initialized = !Dispose();
  }
  return !is_initialized;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    CreateSnapshot
 * Signature: (Ljava/lang/String;[Ljava/lang/Object;Ljava/lang/String;)Z
 */
ALIGN_FUNCTION JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_CreateSnapshot(JNIEnv* env,
                                                                                       jclass cls,
                                                                                       jstring jconfig,
                                                                                       jobjectArray jplugins,
                                                                                       jstring jversion) {
  auto config = Jstring2String(env, jconfig);
  auto version = Jstring2String(env, jversion);
  std::vector<PluginFile> plugin_list;
  const size_t plugin_len = env->GetArrayLength(jplugins);
  for (int i = 0; i < plugin_len; i++) {
    jobjectArray plugin = (jobjectArray)env->GetObjectArrayElement(jplugins, i);
    if (plugin == nullptr) {
      continue;
    }
    jstring jname = (jstring)env->GetObjectArrayElement(plugin, 0);
    jstring jsource = (jstring)env->GetObjectArrayElement(plugin, 1);
    if (jname == nullptr || jsource == nullptr) {
      continue;
    }
    auto name = Jstring2String(env, jname);
    auto source = Jstring2String(env, jsource);
    plugin_list.emplace_back(name, source);
  }
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  Snapshot* blob = new Snapshot(config, plugin_list, version, millis, env);
  if (!blob->IsOk()) {
    delete blob;
    return false;
  }
  std::lock_guard<std::mutex> lock(snapshot_mtx);
  delete snapshot;
  snapshot = blob;
  return true;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Check
 * Signature: (Ljava/lang/String;[BILcom/baidu/openrasp/v8/Context;I)[B
 */
ALIGN_FUNCTION JNIEXPORT jbyteArray JNICALL Java_com_baidu_openrasp_v8_V8_Check(JNIEnv* env,
                                                                                jclass cls,
                                                                                jstring jtype,
                                                                                jbyteArray jparams,
                                                                                jint jparams_size,
                                                                                jobject jcontext,
                                                                                jint jtimeout) {
  Isolate* isolate = per_thread_runtime.GetIsolate();
  if (!isolate) {
    return nullptr;
  }
  v8::Locker lock(isolate);
  if (isolate->IsDead()) {
    return nullptr;
  }
  auto data = isolate->GetData();
  data->custom_data = env;
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  v8::Local<v8::Context> context = data->context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::Local<v8::String> request_type;
  v8::Local<v8::Object> request_params;
  v8::Local<v8::Object> request_context;
  v8::Local<v8::Array> rst;
  std::string type;

  {
    type = Jstring2String(env, jtype);
    if (!v8::String::NewFromUtf8(isolate, type.data(), v8::NewStringType::kInternalized, type.size())
             .ToLocal(&request_type)) {
      return nullptr;
    }
  }

  if (data->check_points.find(type) != data->check_points.end()) {
    {
      auto maybe_string =
          v8::String::NewExternalOneByte(isolate, new ExternalOneByteStringResource(env, jparams, jparams_size));
      if (maybe_string.IsEmpty()) {
        return nullptr;
      }
      auto maybe_obj = v8::JSON::Parse(context, maybe_string.ToLocalChecked());
      if (maybe_obj.IsEmpty()) {
        plugin_log(Exception(isolate, try_catch));
        return nullptr;
      }
      request_params = maybe_obj.ToLocalChecked().As<v8::Object>();
      request_params->SetLazyDataProperty(context, NewV8Key(isolate, "stack", 5), GetStack).IsJust();
    }

    request_context = per_thread_runtime.request_context.Get(isolate);
    if (type == "request" || request_context.IsEmpty()) {
      if (!data->request_context_templ.Get(isolate)->NewInstance(context).ToLocal(&request_context)) {
        return nullptr;
      }
      per_thread_runtime.request_context.Reset(isolate, request_context);
      if (data->hs.used_heap_size() > 10 * 1024 * 1024) {
        per_thread_runtime.request_context.SetWeak();
      }
    }
    request_context->SetInternalField(0, v8::External::New(isolate, jcontext));

    rst = isolate->Check(request_type, request_params, request_context, jtimeout);
  }

  if (type == "requestEnd") {
    per_thread_runtime.request_context.Reset();
  }

  if (rst.IsEmpty() || rst->Length() == 0) {
    return nullptr;
  }

  v8::Local<v8::String> json;
  if (!v8::JSON::Stringify(context, rst).ToLocal(&json)) {
    plugin_log(Exception(isolate, try_catch));
    return nullptr;
  }

  v8::String::Utf8Value val(isolate, json);
  auto bytearray = env->NewByteArray(val.length());
  if (bytearray == nullptr) {
    return nullptr;
  }
  env->SetByteArrayRegion(bytearray, 0, val.length(), reinterpret_cast<jbyte*>(*val));
  return bytearray;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    ExecuteScript
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 * 尽量在脚本中返回字符串类型，因为v8::JSON::Stringify在序列化大对象时可能会导致崩溃
 */
ALIGN_FUNCTION JNIEXPORT jstring JNICALL Java_com_baidu_openrasp_v8_V8_ExecuteScript(JNIEnv* env,
                                                                                     jclass cls,
                                                                                     jstring jsource,
                                                                                     jstring jfilename) {
  Isolate* isolate = per_thread_runtime.GetIsolate();
  if (!isolate) {
    jclass ExceptionClass = env->FindClass("java/lang/Exception");
    env->ThrowNew(ExceptionClass, "Get v8 isolate failed");
    return nullptr;
  }
  v8::Locker lock(isolate);
  if (isolate->IsDead()) {
    jclass ExceptionClass = env->FindClass("java/lang/Exception");
    env->ThrowNew(ExceptionClass, "V8 isolate is dead");
    return nullptr;
  }
  auto data = isolate->GetData();
  data->custom_data = env;
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = data->context.Get(isolate);
  v8::Context::Scope context_scope(context);
  v8::TryCatch try_catch(isolate);
  auto maybe_rst = isolate->ExecScript(Jstring2V8string(env, jsource).FromMaybe(v8::String::Empty(isolate)),
                                       Jstring2V8string(env, jfilename).FromMaybe(v8::String::Empty(isolate)),
                                       v8::Integer::New(isolate, 0));
  if (maybe_rst.IsEmpty()) {
    Exception e(isolate, try_catch);
    jclass ExceptionClass = env->FindClass("java/lang/Exception");
    env->ThrowNew(ExceptionClass, e.c_str());
    return nullptr;
  }
  return V8value2Jstring(env, maybe_rst.ToLocalChecked());
}