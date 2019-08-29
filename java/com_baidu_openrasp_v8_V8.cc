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

using namespace openrasp;

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Initialize
 * Signature: ()Z
 */
ALIGN_FUNCTION JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_Initialize(JNIEnv* env, jclass cls) {
  if (!is_initialized) {
    v8_class = V8Class(env);
    ctx_class = ContextClass(env);
    is_initialized = Initialize(0);
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
  std::unique_lock<std::mutex> lock(snapshot_mtx);
  delete snapshot;
  snapshot = blob;
  return true;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Check
 * Signature: (Ljava/lang/String;[BILcom/baidu/openrasp/v8/Context;ZI)[B
 */
ALIGN_FUNCTION JNIEXPORT jbyteArray JNICALL Java_com_baidu_openrasp_v8_V8_Check(JNIEnv* env,
                                                                                jclass cls,
                                                                                jstring jtype,
                                                                                jbyteArray jparams,
                                                                                jint jparams_size,
                                                                                jobject jcontext,
                                                                                jboolean jnew_request,
                                                                                jint jtimeout) {
  Isolate* isolate = GetIsolate(env);
  if (!isolate) {
    return nullptr;
  }
  auto data = isolate->GetData();

  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::String> request_type;
  v8::Local<v8::Object> request_params;
  v8::Local<v8::Object> request_context;

  {
    if (!Jstring2V8string(env, jtype).ToLocal(&request_type)) {
      return nullptr;
    }
  }

  {
    auto raw = static_cast<uint8_t*>(env->GetPrimitiveArrayCritical(jparams, nullptr));
    // https://stackoverflow.com/questions/36101913/should-i-always-call-releaseprimitivearraycritical-even-if-getprimitivearraycrit
    if (raw == nullptr) {
      return nullptr;
    }
    auto maybe_string = v8::String::NewFromOneByte(isolate, raw, v8::NewStringType::kNormal, jparams_size);
    env->ReleasePrimitiveArrayCritical(jparams, raw, JNI_ABORT);
    if (maybe_string.IsEmpty()) {
      return nullptr;
    }
    auto maybe_obj = v8::JSON::Parse(context, maybe_string.ToLocalChecked());
    if (maybe_obj.IsEmpty()) {
      return nullptr;
    }
    request_params = maybe_obj.ToLocalChecked().As<v8::Object>();
    request_params->SetLazyDataProperty(context, NewV8Key(isolate, "stack", 5), GetStack).IsJust();
  }

  if (jnew_request || data->request_context.Get(isolate).IsEmpty()) {
    request_context = data->request_context_templ.Get(isolate)->NewInstance(context).FromMaybe(v8::Object::New(isolate));
    data->request_context.Reset(isolate, request_context);
  } else {
    request_context = data->request_context.Get(isolate);
  }
  request_context->SetInternalField(0, v8::External::New(isolate, jcontext));

  auto rst = isolate->Check(request_type, request_params, request_context, jtimeout);

  if (rst->Length() == 0) {
    return nullptr;
  }

  v8::Local<v8::String> json;
  if (!v8::JSON::Stringify(context, rst).ToLocal(&json)) {
    return nullptr;
  }

  auto bytearray = env->NewByteArray(json->Utf8Length(isolate));
  if (bytearray == nullptr) {
    return nullptr;
  }
  auto bytes = env->GetPrimitiveArrayCritical(bytearray, nullptr);
  if (bytes == nullptr) {
    return nullptr;
  }
  json->WriteUtf8(isolate, static_cast<char*>(bytes));
  env->ReleasePrimitiveArrayCritical(bytearray, bytes, 0);
  return bytearray;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    ExecuteScript
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 * 尽量在脚本中返回字符串类型，因为v8::JSON::Stringify在序列化大对象时可能会导致崩溃
 *
 * 暂时地:jstinrg->uint16_t*->std::u16string->std::string
 * 下个版本应该在Isolate类中增加ExecScript(v8::Local<v8::String>, ...)，以便免去转换过程
 * 不直接用env->GetStringUTFChars的原因是jni不能正确转换一些特殊字符到utf8
 */
ALIGN_FUNCTION JNIEXPORT jstring JNICALL Java_com_baidu_openrasp_v8_V8_ExecuteScript(JNIEnv* env,
                                                                                     jclass cls,
                                                                                     jstring jsource,
                                                                                     jstring jfilename) {
  Isolate* isolate = GetIsolate(env);
  if (!isolate) {
    jclass ExceptionClass = env->FindClass("java/lang/Exception");
    env->ThrowNew(ExceptionClass, "Get v8 isolate failed");
    return nullptr;
  }
  auto data = isolate->GetData();
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  std::string source = Jstring2String(env, jsource);
  std::string filename = Jstring2String(env, jfilename);
  auto maybe_rst = isolate->ExecScript(source, filename);
  if (maybe_rst.IsEmpty()) {
    Exception e(isolate, try_catch);
    jclass ExceptionClass = env->FindClass("java/lang/Exception");
    env->ThrowNew(ExceptionClass, e.c_str());
    return nullptr;
  }
  return V8value2Jstring(env, maybe_rst.ToLocalChecked());
}