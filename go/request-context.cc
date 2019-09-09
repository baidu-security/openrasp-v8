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

#include "base/bundle.h"
#include "header.h"
#include "string.h"

using namespace openrasp_v8;

void* CreateV8String(void* isolate, Buffer buf) {
  auto maybe =
      v8::String::NewFromUtf8(reinterpret_cast<Isolate*>(isolate), *buf, v8::NewStringType::kNormal, buf.length());
  return new v8::MaybeLocal<v8::String>(maybe);
}

void* CreateV8ArrayBuffer(void* isolate, Buffer buf) {
  auto ab = v8::ArrayBuffer::New(reinterpret_cast<Isolate*>(isolate), buf.raw_size);
  auto contents = ab->GetContents();
  memcpy(contents.Data(), buf.data, buf.raw_size);
  return new v8::MaybeLocal<v8::ArrayBuffer>(ab);
}

static void url_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr = reinterpret_cast<v8::MaybeLocal<v8::String>*>(urlGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

static void path_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(pathGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

static void method_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(methodGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

static void querystring_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(querystringGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

static void protocol_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(protocolGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

static void remoteAddr_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(remoteAddrGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

static void appBasePath_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(appBasePathGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

static void header_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto obj = v8::Object::New(isolate).As<v8::Value>();
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(headerGetter((void*)isolate, custom_data->context_index));

  if (maybe_ptr && !maybe_ptr->IsEmpty()) {
    obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_ptr->ToLocalChecked()).FromMaybe(obj);
  }
  returnValue.Set(obj);

  delete maybe_ptr;
}

static void parameter_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto obj = v8::Object::New(isolate).As<v8::Value>();
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(parameterGetter((void*)isolate, custom_data->context_index));

  if (maybe_ptr && !maybe_ptr->IsEmpty()) {
    obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_ptr->ToLocalChecked()).FromMaybe(obj);
  }
  returnValue.Set(obj);

  delete maybe_ptr;
}

static void json_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto obj = v8::Object::New(isolate).As<v8::Value>();
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(jsonGetter((void*)isolate, custom_data->context_index));

  if (maybe_ptr && !maybe_ptr->IsEmpty()) {
    obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_ptr->ToLocalChecked()).FromMaybe(obj);
  }
  returnValue.Set(obj);

  delete maybe_ptr;
}

static void server_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto obj = v8::Object::New(isolate).As<v8::Value>();
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::String>*>(serverGetter((void*)isolate, custom_data->context_index));

  if (maybe_ptr && !maybe_ptr->IsEmpty()) {
    obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe_ptr->ToLocalChecked()).FromMaybe(obj);
  }
  returnValue.Set(obj);

  delete maybe_ptr;
}

static void body_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto custom_data = GetCustomData(isolate);
  auto maybe_ptr =
      reinterpret_cast<v8::MaybeLocal<v8::ArrayBuffer>*>(bodyGetter((void*)isolate, custom_data->context_index));

  if (!maybe_ptr || maybe_ptr->IsEmpty()) {
    returnValue.Set(v8::ArrayBuffer::New(isolate, 0));
  } else {
    returnValue.Set(maybe_ptr->ToLocalChecked());
  }

  delete maybe_ptr;
}

v8::Local<v8::ObjectTemplate> openrasp_v8::CreateRequestContextTemplate(Isolate* isolate) {
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
  return obj_templ;
}
