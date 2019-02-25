#include "header.h"

using namespace openrasp;

JavaVM *jvm = nullptr;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
  JNIEnv *env;
  if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) != JNI_OK || !env)
  {
    return -1;
  }
  jvm = vm;
  v8_class = V8Class(env);
  ctx_class = ContextClass(env);
  Java_com_baidu_openrasp_plugin_v8_V8_Initialize(env, nullptr);
  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved)
{
  JNIEnv *env;
  if (vm->GetEnv((void **)&env, JNI_VERSION_1_6) == JNI_OK && !env)
  {
    Java_com_baidu_openrasp_plugin_v8_V8_Dispose(env, nullptr);
  }
}

/*
 * Class:     com_baidu_openrasp_plugin_v8_V8
 * Method:    Initialize
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_plugin_v8_V8_Initialize(JNIEnv *env, jclass cls)
{
  if (!isInitialized)
  {
    Platform::Initialize();
    v8::V8::Initialize();
    isInitialized = true;
  }
  return isInitialized;
}

/*
 * Class:     com_baidu_openrasp_plugin_v8_V8
 * Method:    Dispose
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_plugin_v8_V8_Dispose(JNIEnv *env, jclass cls)
{
  if (isInitialized)
  {
    delete snapshot;
    Platform::Shutdown();
    v8::V8::Dispose();
    isInitialized = false;
  }
  return !isInitialized;
}

/*
 * Class:     com_baidu_openrasp_plugin_v8_V8
 * Method:    CreateSnapshot
 * Signature: (Ljava/lang/String;[Ljava/lang/Object;)Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_plugin_v8_V8_CreateSnapshot(JNIEnv *env, jclass cls, jstring jconfig, jobjectArray jplugins)
{
  const char *raw_config = env->GetStringUTFChars(jconfig, 0);
  std::string config(raw_config);
  env->ReleaseStringUTFChars(jconfig, raw_config);

  std::vector<PluginFile> plugin_list;
  const size_t plugin_len = env->GetArrayLength(jplugins);
  for (int i = 0; i < plugin_len; i++)
  {
    jobjectArray plugin = (jobjectArray)env->GetObjectArrayElement(jplugins, i);
    jstring name = (jstring)env->GetObjectArrayElement(plugin, 0);
    jstring source = (jstring)env->GetObjectArrayElement(plugin, 1);
    const char *raw_name = env->GetStringUTFChars(name, 0);
    const char *raw_source = env->GetStringUTFChars(source, 0);
    plugin_list.emplace_back(raw_name, raw_source);
    env->ReleaseStringUTFChars(name, raw_name);
    env->ReleaseStringUTFChars(source, raw_source);
  }
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  CustomData custom_data;
  custom_data.env = env;
  Snapshot *blob = new Snapshot(config, plugin_list, millis, &custom_data);
  if (!blob->IsOk())
  {
    delete blob;
  }
  else
  {
    std::lock_guard<std::mutex> lock(mtx);
    delete snapshot;
    snapshot = blob;
  }
  return true;
}

/*
 * Class:     com_baidu_openrasp_plugin_v8_V8
 * Method:    Check
 * Signature: (Ljava/lang/String;[BILcom/baidu/openrasp/plugin/v8/Context;Z)Z
 */
JNIEXPORT jboolean JNICALL Java_com_baidu_openrasp_plugin_v8_V8_Check(JNIEnv *env, jclass cls, jstring jtype, jbyteArray jparams, jint jparams_size, jobject jcontext, jboolean jnew_request)
{
  Isolate *isolate = GetIsolate();
  if (!isolate)
  {
    return false;
  }
  auto data = isolate->GetData();
  GetCustomData(isolate)->env = env;
  GetCustomData(isolate)->context = jcontext;

  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> type;
  v8::Local<v8::Object> request_params;

  {
    const jchar *raw = env->GetStringCritical(jtype, nullptr);
    const size_t len = env->GetStringLength(jtype);
    bool rst = v8::String::NewFromTwoByte(isolate, raw, v8::NewStringType::kNormal, len).ToLocal(&type);
    env->ReleaseStringCritical(jtype, raw);
    if (!rst)
    {
      return false;
    }
  }

  {
    char *raw = static_cast<char *>(env->GetPrimitiveArrayCritical(jparams, nullptr));
    auto maybe_string = v8::String::NewFromUtf8(isolate, raw, v8::NewStringType::kNormal, jparams_size);
    env->ReleasePrimitiveArrayCritical(jparams, raw, JNI_ABORT);
    if (maybe_string.IsEmpty())
    {
      return false;
    }
    auto maybe_obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
    if (maybe_obj.IsEmpty())
    {
      return false;
    }
    request_params = maybe_obj.ToLocalChecked().As<v8::Object>();
  }

  if (jnew_request)
  {
    auto request_context = data->request_context_templ.Get(isolate)->NewInstance();
    isolate->GetData()->request_context.Reset(isolate, request_context);
  }

  return isolate->Check(type, request_params);
}