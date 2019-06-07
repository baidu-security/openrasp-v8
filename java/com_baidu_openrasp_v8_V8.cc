#include "header.h"

using namespace openrasp;

/*
 * Class:     com_baidu_openrasp_v8_V8
 * Method:    Initialize
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_v8_V8_Initialize(JNIEnv* env, jclass cls) {
  if (!isInitialized) {
    v8::V8::InitializePlatform(Platform::New(0));
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
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
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
  auto config = Jstring2String(env, jconfig);

  std::vector<PluginFile> plugin_list;
  const size_t plugin_len = env->GetArrayLength(jplugins);
  for (int i = 0; i < plugin_len; i++) {
    jobjectArray plugin = (jobjectArray)env->GetObjectArrayElement(jplugins, i);
    jstring jname = (jstring)env->GetObjectArrayElement(plugin, 0);
    jstring jsource = (jstring)env->GetObjectArrayElement(plugin, 1);
    auto name = Jstring2String(env, jname);
    auto source = Jstring2String(env, jsource);
    plugin_list.emplace_back(name, source);
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
    uint8_t* raw = static_cast<uint8_t*>(env->GetPrimitiveArrayCritical(jparams, nullptr));
    auto maybe_string = v8::String::NewFromOneByte(isolate, raw, v8::NewStringType::kNormal, jparams_size);
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
 *
 * 暂时地:jstinrg->uint16_t*->std::u16string->std::string
 * 下个版本应该在Isolate类中增加ExecScript(v8::Local<v8::String>, ...)，以便免去转换过程
 * 不直接用env->GetStringUTFChars的原因是jni不能正确转换一些特殊字符到utf8
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
  auto data = isolate->GetData();
  auto custom_data = GetCustomData(isolate);
  custom_data->env = env;
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
  v8::String::Value string_value(isolate, maybe_rst.ToLocalChecked());
  return env->NewString(*string_value, string_value.length());
}