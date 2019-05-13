#include "header.h"

using namespace openrasp;

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Initialize
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_Initialize(JNIEnv* env, jclass cls) {
  if (!isInitialized) {
    Platform::Initialize();
    v8::V8::Initialize();
    v8_class = V8Class(env);
    ctx_class = ContextClass(env);
    isInitialized = true;
  }
  return isInitialized;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Dispose
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_Dispose(JNIEnv* env, jclass cls) {
  if (isInitialized) {
    delete snapshot;
    Platform::Shutdown();
    v8::V8::Dispose();
    isInitialized = false;
  }
  return !isInitialized;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    CreateSnapshot
 * Signature: (Ljava/lang/String;[Ljava/lang/Object;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_CreateSnapshot(JNIEnv* env,
                                                                        jclass cls,
                                                                        jstring jconfig,
                                                                        jobjectArray jplugins) {
  const char* raw_config = env->GetStringUTFChars(jconfig, 0);
  std::string config(raw_config);
  env->ReleaseStringUTFChars(jconfig, raw_config);

  std::vector<PluginFile> plugin_list;
  const size_t plugin_len = env->GetArrayLength(jplugins);
  for (int i = 0; i < plugin_len; i++) {
    jobjectArray plugin = (jobjectArray)env->GetObjectArrayElement(jplugins, i);
    jstring name = (jstring)env->GetObjectArrayElement(plugin, 0);
    jstring source = (jstring)env->GetObjectArrayElement(plugin, 1);
    const char* raw_name = env->GetStringUTFChars(name, 0);
    const char* raw_source = env->GetStringUTFChars(source, 0);
    plugin_list.emplace_back(raw_name, raw_source);
    env->ReleaseStringUTFChars(name, raw_name);
    env->ReleaseStringUTFChars(source, raw_source);
  }
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  CustomData custom_data;
  custom_data.env = env;
  Snapshot* blob = new Snapshot(config, plugin_list, millis, &custom_data);
  if (!blob->IsOk()) {
    return false;
    delete blob;
  }
  std::lock_guard<std::mutex> lock(mtx);
  delete snapshot;
  snapshot = blob;
  return true;
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Check
 * Signature: (Ljava/lang/String;[BILcom/baidu/openrasp/plugin/v8/Context;Z)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_baidu_openrasp_v8_V8_Check(JNIEnv* env,
                                                              jclass cls,
                                                              jstring jtype,
                                                              jbyteArray jparams,
                                                              jint jparams_size,
                                                              jobject jcontext,
                                                              jboolean jnew_request) {
  Isolate* isolate = GetIsolate();
  if (!isolate) {
    return nullptr;
  }
  auto data = isolate->GetData();
  auto custom_data = GetCustomData(isolate);
  custom_data->env = env;
  custom_data->context = jcontext;

  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> request_type;
  v8::Local<v8::Object> request_params;
  v8::Local<v8::Object> request_context;

  {
    const jchar* raw = env->GetStringCritical(jtype, nullptr);
    const size_t len = env->GetStringLength(jtype);
    bool rst = v8::String::NewFromTwoByte(isolate, raw, v8::NewStringType::kNormal, len).ToLocal(&request_type);
    env->ReleaseStringCritical(jtype, raw);
    if (!rst) {
      return nullptr;
    }
  }

  {
    char* raw = static_cast<char*>(env->GetPrimitiveArrayCritical(jparams, nullptr));
    auto maybe_string = v8::String::NewFromUtf8(isolate, raw, v8::NewStringType::kNormal, jparams_size);
    env->ReleasePrimitiveArrayCritical(jparams, raw, JNI_ABORT);
    if (maybe_string.IsEmpty()) {
      return nullptr;
    }
    auto maybe_obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
    if (maybe_obj.IsEmpty()) {
      return nullptr;
    }
    request_params = maybe_obj.ToLocalChecked().As<v8::Object>();
  }

  if (jnew_request || data->request_context.Get(isolate).IsEmpty()) {
    request_context = data->request_context_templ.Get(isolate)->NewInstance();
    data->request_context.Reset(isolate, request_context);
  } else {
    request_context = data->request_context.Get(isolate);
  }

  auto rst = isolate->Check(request_type, request_params, request_context);

  if (rst->Length() == 0) {
    return nullptr;
  }

  v8::Local<v8::String> json;
  if (!v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocal(&json)) {
    return nullptr;
  }

  v8::String::Value string_value(isolate, json);
  return env->NewString(*string_value, string_value.length());
}

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    ExecuteScript
 * Signature: (Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;
 * 尽量在脚本中返回字符串类型，因为v8::JSON::Stringify在序列化大对象时可能会导致崩溃
 */
JNIEXPORT jstring JNICALL Java_com_baidu_openrasp_v8_V8_ExecuteScript(JNIEnv* env,
                                                                      jclass cls,
                                                                      jstring jsource,
                                                                      jstring jfilename) {
  Isolate* isolate = GetIsolate();
  if (!isolate) {
    jclass ExceptionClass = env->FindClass("java/lang/Exception");
    env->ThrowNew(ExceptionClass, "Get v8 isolate failed");
    return nullptr;
  }
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  const char* source = env->GetStringUTFChars(jsource, nullptr);
  const size_t source_len = env->GetStringLength(jsource);
  const char* filename = env->GetStringUTFChars(jfilename, nullptr);
  const size_t filename_len = env->GetStringLength(jfilename);
  auto maybe_rst = isolate->ExecScript({source, source_len}, {filename, filename_len});
  env->ReleaseStringUTFChars(jsource, source);
  env->ReleaseStringUTFChars(jfilename, filename);
  if (maybe_rst.IsEmpty()) {
    Exception e(isolate, try_catch);
    jclass ExceptionClass = env->FindClass("java/lang/Exception");
    env->ThrowNew(ExceptionClass, e.c_str());
    return nullptr;
  }
  v8::String::Value string_value(isolate, maybe_rst.ToLocalChecked());
  return env->NewString(*string_value, string_value.length());
}