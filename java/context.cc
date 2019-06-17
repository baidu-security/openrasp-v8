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
#include "header.h"

using namespace openrasp;

enum FieldIndex {
  kUrl = 0,
  kHeader,
  kParameter,
  kPath,
  kQuerystring,
  kMethod,
  kProtocol,
  kRemoteAddr,
  kAppBasePath,
  kBody,
  kServer,
  kJson,
  kRequestId,
  kEndForCount
};
inline Isolate* GetIsolate(const v8::PropertyCallbackInfo<v8::Value>& info) {
  return reinterpret_cast<Isolate*>(info.GetIsolate());
}
static void url_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kUrl);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getUrl);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kUrl, obj);
}
static void method_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kMethod);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getMethod);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kMethod, obj);
}
static void querystring_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kQuerystring);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getQuerystring);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kQuerystring, obj);
}
static void appBasePath_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kAppBasePath);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getAppBasePath);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kAppBasePath, obj);
}
static void protocol_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kProtocol);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getProtocol);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kProtocol, obj);
}
static void remoteAddr_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kRemoteAddr);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getRemoteAddr);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kRemoteAddr, obj);
}
static void path_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kPath);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getPath);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kPath, obj);
}
static void requestId_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kRequestId);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.SetEmptyString();

  auto custom_data = GetCustomData(isolate);
  jstring jstr = (jstring)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getRequestId);
  if (!jstr) {
    return;
  }
  auto maybe_string = Jstring2V8string(custom_data->env, jstr);
  if (maybe_string.IsEmpty()) {
    return;
  }

  auto obj = maybe_string.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kRequestId, obj);
}
static void parameter_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kParameter);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.Set(v8::Object::New(isolate));

  auto custom_data = GetCustomData(isolate);
  jintArray jsize = custom_data->env->NewIntArray(1);
  jbyteArray jbuffer =
      (jbyteArray)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getParameter, jsize);
  if (!jbuffer) {
    return;
  }
  jint jbuffer_size;
  custom_data->env->GetIntArrayRegion(jsize, 0, 1, &jbuffer_size);
  auto raw = static_cast<uint8_t*>(custom_data->env->GetPrimitiveArrayCritical(jbuffer, nullptr));
  auto maybe_string = v8::String::NewFromOneByte(isolate, raw, v8::NewStringType::kNormal, jbuffer_size);
  custom_data->env->ReleasePrimitiveArrayCritical(jbuffer, raw, JNI_ABORT);
  if (maybe_string.IsEmpty()) {
    return;
  }
  auto maybe_obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
  if (maybe_obj.IsEmpty()) {
    return;
  }

  auto obj = maybe_obj.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kParameter, obj);
}
static void header_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kHeader);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.Set(v8::Object::New(isolate));

  auto custom_data = GetCustomData(isolate);
  jintArray jsize = custom_data->env->NewIntArray(1);
  jbyteArray jbuffer = (jbyteArray)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getHeader, jsize);
  if (!jbuffer) {
    return;
  }
  jint jbuffer_size;
  custom_data->env->GetIntArrayRegion(jsize, 0, 1, &jbuffer_size);
  auto raw = static_cast<uint8_t*>(custom_data->env->GetPrimitiveArrayCritical(jbuffer, nullptr));
  auto maybe_string = v8::String::NewFromOneByte(isolate, raw, v8::NewStringType::kNormal, jbuffer_size);
  custom_data->env->ReleasePrimitiveArrayCritical(jbuffer, raw, JNI_ABORT);
  if (maybe_string.IsEmpty()) {
    return;
  }
  auto maybe_obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
  if (maybe_obj.IsEmpty()) {
    return;
  }

  auto obj = maybe_obj.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kHeader, obj);
}
static void body_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kBody);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.Set(v8::ArrayBuffer::New(isolate, 0));

  auto custom_data = GetCustomData(isolate);
  jintArray jsize = custom_data->env->NewIntArray(1);
  jbyteArray jbuffer = (jbyteArray)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getBody, jsize);
  if (!jbuffer) {
    return;
  }
  jint jbuffer_size;
  custom_data->env->GetIntArrayRegion(jsize, 0, 1, &jbuffer_size);
  auto raw = static_cast<uint8_t*>(custom_data->env->GetPrimitiveArrayCritical(jbuffer, nullptr));
  auto body = v8::ArrayBuffer::New(isolate, raw, jbuffer_size);
  custom_data->env->ReleasePrimitiveArrayCritical(jbuffer, raw, JNI_ABORT);

  returnValue.Set(body);
  self->SetInternalField(kBody, body);
}
static void server_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kServer);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.Set(v8::Object::New(isolate));

  auto custom_data = GetCustomData(isolate);
  jintArray jsize = custom_data->env->NewIntArray(1);
  jbyteArray jbuffer = (jbyteArray)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getServer, jsize);
  if (!jbuffer) {
    return;
  }
  jint jbuffer_size;
  custom_data->env->GetIntArrayRegion(jsize, 0, 1, &jbuffer_size);
  auto raw = static_cast<uint8_t*>(custom_data->env->GetPrimitiveArrayCritical(jbuffer, nullptr));
  auto maybe_string = v8::String::NewFromOneByte(isolate, raw, v8::NewStringType::kNormal, jbuffer_size);
  custom_data->env->ReleasePrimitiveArrayCritical(jbuffer, raw, JNI_ABORT);
  if (maybe_string.IsEmpty()) {
    return;
  }
  auto maybe_obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
  if (maybe_obj.IsEmpty()) {
    return;
  }

  auto obj = maybe_obj.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kServer, obj);
}

static void json_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto self = info.Holder();
  auto returnValue = info.GetReturnValue();
  auto cache = self->GetInternalField(kJson);
  if (!cache->IsUndefined()) {
    returnValue.Set(cache);
    return;
  }

  auto isolate = GetIsolate(info);
  returnValue.Set(v8::Object::New(isolate));

  auto custom_data = GetCustomData(isolate);
  jintArray jsize = custom_data->env->NewIntArray(1);
  jbyteArray jbuffer = (jbyteArray)custom_data->env->CallObjectMethod(custom_data->context, ctx_class.getJson, jsize);
  if (!jbuffer) {
    return;
  }
  jint jbuffer_size;
  custom_data->env->GetIntArrayRegion(jsize, 0, 1, &jbuffer_size);
  auto raw = static_cast<uint8_t*>(custom_data->env->GetPrimitiveArrayCritical(jbuffer, nullptr));
  auto maybe_string = v8::String::NewFromOneByte(isolate, raw, v8::NewStringType::kNormal, jbuffer_size);
  custom_data->env->ReleasePrimitiveArrayCritical(jbuffer, raw, JNI_ABORT);
  if (maybe_string.IsEmpty()) {
    return;
  }
  auto maybe_obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_string.ToLocalChecked());
  if (maybe_obj.IsEmpty()) {
    return;
  }

  auto obj = maybe_obj.ToLocalChecked();
  returnValue.Set(obj);
  self->SetInternalField(kJson, obj);
}

v8::Local<v8::ObjectTemplate> openrasp::CreateRequestContextTemplate(Isolate* isolate) {
  auto obj_templ = v8::ObjectTemplate::New(isolate);
  obj_templ->SetAccessor(NewV8String(isolate, "url"), url_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "header"), header_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "parameter"), parameter_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "path"), path_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "querystring"), querystring_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "method"), method_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "protocol"), protocol_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "remoteAddr"), remoteAddr_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "appBasePath"), appBasePath_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "body"), body_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "server"), server_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "json"), json_getter);
  obj_templ->SetAccessor(NewV8String(isolate, "requestId"), requestId_getter);
  obj_templ->SetInternalFieldCount(kEndForCount);
  return obj_templ;
}