#include <cpr/cpr.h>
#include <algorithm>
#include <string>
#include "bundle.h"

namespace openrasp {

void request_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;
  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    try_catch.ReThrow();
    return;
  }
  info.GetReturnValue().Set(resolver->GetPromise());
  v8::Local<v8::Object> config;
  if (!info[0]->ToObject(context).ToLocal(&config)) {
    resolver->Reject(context, try_catch.Exception()).FromJust();
    return;
  }
  cpr::Session session;
  cpr::Header header;
  std::string method;
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8String(isolate, "method")).FromMaybe(v8::Local<v8::Value>());
    if (!tmp.IsEmpty() && tmp->IsString()) {
      method = *v8::String::Utf8Value(isolate, tmp);
      std::transform(method.begin(), method.end(), method.begin(), ::tolower);
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8String(isolate, "url")).FromMaybe(v8::Local<v8::Value>());
    if (!tmp.IsEmpty() && tmp->IsString()) {
      session.SetUrl(*v8::String::Utf8Value(isolate, tmp));
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8String(isolate, "params")).FromMaybe(v8::Local<v8::Value>());
    if (!tmp.IsEmpty() && tmp->IsObject()) {
      auto object = tmp.As<v8::Object>();
      v8::Local<v8::Array> props = object->GetOwnPropertyNames(context).ToLocalChecked();
      cpr::Parameters parameters;
      for (int i = 0; i < props->Length(); i++) {
        auto key = props->Get(context, i).ToLocalChecked();
        auto val = object->Get(context, key).ToLocalChecked();
        parameters.AddParameter({*v8::String::Utf8Value(isolate, key), *v8::String::Utf8Value(isolate, val)});
      }
      session.SetParameters(parameters);
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8String(isolate, "data")).FromMaybe(v8::Local<v8::Value>());
    if (!tmp.IsEmpty()) {
      if (tmp->IsObject()) {
        auto json = v8::JSON::Stringify(context, tmp).FromMaybe(v8::String::Empty(isolate));
        session.SetBody(cpr::Body(*v8::String::Utf8Value(isolate, json)));
        header.emplace("content-type", "application/json");
      } else if (tmp->IsString()) {
        session.SetBody(cpr::Body(*v8::String::Utf8Value(isolate, tmp)));
      } else if (tmp->IsArrayBuffer()) {
        auto arraybuffer = tmp.As<v8::ArrayBuffer>();
        auto content = arraybuffer->GetContents();
        session.SetBody(cpr::Body(static_cast<const char*>(content.Data()), content.ByteLength()));
        header.emplace("content-type", "application/octet-stream");
      }
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8String(isolate, "maxRedirects")).FromMaybe(v8::Local<v8::Value>());
    if (!tmp.IsEmpty() && tmp->IsInt32()) {
      session.SetMaxRedirects(cpr::MaxRedirects(tmp->Int32Value(context).FromJust()));
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8String(isolate, "timeout")).FromMaybe(v8::Local<v8::Value>());
    if (!tmp.IsEmpty() && tmp->IsInt32()) {
      session.SetTimeout(tmp->Int32Value(context).FromJust());
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8String(isolate, "headers")).FromMaybe(v8::Local<v8::Value>());
    if (!tmp.IsEmpty() && tmp->IsObject()) {
      auto object = tmp.As<v8::Object>();
      v8::Local<v8::Array> props = object->GetOwnPropertyNames(context).ToLocalChecked();
      for (int i = 0; i < props->Length(); i++) {
        auto key = props->Get(context, i).ToLocalChecked();
        auto val = object->Get(context, key).ToLocalChecked();
        header.emplace(*v8::String::Utf8Value(isolate, key), *v8::String::Utf8Value(isolate, val));
      }
      session.SetHeader(header);
    }
  }
  session.SetVerifySsl(false);
  cpr::Response r;
  if (method == "get") {
    r = session.Get();
  } else if (method == "post") {
    r = session.Post();
  } else if (method == "put") {
    r = session.Put();
  } else if (method == "patch") {
    r = session.Patch();
  } else if (method == "head") {
    r = session.Head();
  } else if (method == "options") {
    r = session.Options();
  } else if (method == "delete") {
    r = session.Delete();
  } else {
    r = session.Get();
  }

  if (r.error.code == cpr::ErrorCode::OK) {
    auto ret_val = v8::Object::New(isolate);
    ret_val->Set(context, NewV8String(isolate, "status"), v8::Int32::New(isolate, r.status_code)).FromJust();
    auto data = v8::String::NewFromUtf8(isolate, r.text.data(), v8::NewStringType::kNormal, r.text.size())
                    .FromMaybe(v8::String::Empty(isolate));
    ret_val->Set(context, NewV8String(isolate, "data"), data).FromJust();
    auto headers = v8::Object::New(isolate);
    for (auto& h : r.header) {
      auto key = v8::String::NewFromUtf8(isolate, h.first.data(), v8::NewStringType::kNormal, h.first.size());
      auto val = v8::String::NewFromUtf8(isolate, h.second.data(), v8::NewStringType::kNormal, h.second.size());
      if (key.IsEmpty() || val.IsEmpty()) {
        continue;
      }
      headers->Set(context, key.ToLocalChecked(), val.ToLocalChecked()).FromJust();
    }
    ret_val->Set(context, NewV8String(isolate, "headers"), headers).FromJust();
    ret_val->Set(context, NewV8String(isolate, "config"), config).FromJust();
    resolver->Resolve(context, ret_val).FromJust();
  } else {
    auto ret_val = v8::Object::New(isolate);
    auto error = v8::Object::New(isolate);
    error->Set(context, NewV8String(isolate, "code"), v8::Int32::New(isolate, static_cast<int32_t>(r.error.code)))
        .FromJust();
    auto message =
        v8::String::NewFromUtf8(isolate, r.error.message.data(), v8::NewStringType::kNormal, r.error.message.size())
            .FromMaybe(v8::String::Empty(isolate));
    error->Set(context, NewV8String(isolate, "message"), message).FromJust();
    ret_val->Set(context, NewV8String(isolate, "error"), error).FromJust();
    ret_val->Set(context, NewV8String(isolate, "config"), config).FromJust();
    resolver->Reject(context, ret_val).FromJust();
  }
}

}  // namespace openrasp