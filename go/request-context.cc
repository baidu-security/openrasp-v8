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

#include "_cgo_export.h"
#include "base/bundle.h"
#include "export.h"

using namespace openrasp;

inline Isolate* GetIsolate(const v8::PropertyCallbackInfo<v8::Value>& info) {
  return reinterpret_cast<Isolate*>(info.GetIsolate());
}

static void url_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);

  v8::MaybeLocal<v8::String> maybe;
  urlGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void path_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);

  v8::MaybeLocal<v8::String> maybe;
  pathGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void method_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);

  v8::MaybeLocal<v8::String> maybe;
  methodGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void querystring_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);

  v8::MaybeLocal<v8::String> maybe;
  querystringGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void protocol_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);

  v8::MaybeLocal<v8::String> maybe;
  protocolGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void remoteAddr_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);

  v8::MaybeLocal<v8::String> maybe;
  remoteAddrGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void appBasePath_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);

  v8::MaybeLocal<v8::String> maybe;
  appBasePathGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.SetEmptyString();
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void header_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);
  auto obj = v8::Object::New(isolate).As<v8::Value>();

  v8::MaybeLocal<v8::String> maybe;
  headerGetter((void*)isolate, (void*)&maybe);

  if (!maybe.IsEmpty()) {
    obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe.ToLocalChecked()).FromMaybe(obj);
  }

  returnValue.Set(obj);
}

static void parameter_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);
  auto obj = v8::Object::New(isolate).As<v8::Value>();

  v8::MaybeLocal<v8::String> maybe;
  parameterGetter((void*)isolate, (void*)&maybe);

  if (!maybe.IsEmpty()) {
    obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe.ToLocalChecked()).FromMaybe(obj);
  }

  returnValue.Set(obj);
}

static void json_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);
  auto obj = v8::Object::New(isolate).As<v8::Value>();

  v8::MaybeLocal<v8::ArrayBuffer> maybe;
  jsonGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.Set(v8::ArrayBuffer::New(isolate, 0));
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
}

static void server_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);
  auto obj = v8::Object::New(isolate).As<v8::Value>();

  v8::MaybeLocal<v8::String> maybe;
  serverGetter((void*)isolate, (void*)&maybe);

  if (!maybe.IsEmpty()) {
    obj = v8::JSON::Parse(isolate->GetCurrentContext(), maybe.ToLocalChecked()).FromMaybe(obj);
  }

  returnValue.Set(obj);
}

static void body_getter(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value>& info) {
  auto returnValue = info.GetReturnValue();
  auto isolate = GetIsolate(info);
  auto obj = v8::Object::New(isolate).As<v8::Value>();

  v8::MaybeLocal<v8::ArrayBuffer> maybe;
  bodyGetter((void*)isolate, (void*)&maybe);

  if (maybe.IsEmpty()) {
    returnValue.Set(v8::ArrayBuffer::New(isolate, 0));
  } else {
    returnValue.Set(maybe.ToLocalChecked());
  }
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
  return obj_templ;
}