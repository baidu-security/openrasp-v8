#include <cpr/cpr.h>
#include <zlib.h>
#include <algorithm>
#include <string>
#include "bundle.h"

namespace openrasp_v8 {

void request_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
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
    resolver->Reject(context, try_catch.Exception()).IsJust();
    return;
  }
  auto undefined = v8::Undefined(isolate).As<v8::Value>();
  cpr::Session session;
  cpr::Header header;
  cpr::Parameters parameters;
  cpr::Body body;
  std::string method;
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "method")).FromMaybe(undefined);
    if (tmp->IsString()) {
      method = *v8::String::Utf8Value(isolate, tmp);
      std::transform(method.begin(), method.end(), method.begin(), ::tolower);
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "url")).FromMaybe(undefined);
    if (tmp->IsString()) {
      session.SetUrl(*v8::String::Utf8Value(isolate, tmp));
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "params")).FromMaybe(undefined);
    if (tmp->IsObject()) {
      auto object = tmp.As<v8::Object>();
      v8::Local<v8::Array> props;
      if (object->GetOwnPropertyNames(context).ToLocal(&props)) {
        for (int i = 0; i < props->Length(); i++) {
          v8::HandleScope handle_scope(isolate);
          v8::Local<v8::Value> key, val;
          if (props->Get(context, i).ToLocal(&key) && object->Get(context, key).ToLocal(&val)) {
            parameters.AddParameter({*v8::String::Utf8Value(isolate, key), *v8::String::Utf8Value(isolate, val)});
          }
        }
        session.SetParameters(parameters);
      }
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "data")).FromMaybe(undefined);
    if (tmp->IsObject()) {
      v8::Local<v8::Value> json;
      if (!v8::JSON::Stringify(context, tmp).ToLocal(&json)) {
        resolver->Reject(context, try_catch.Exception()).IsJust();
        return;
      }
      body = cpr::Body(*v8::String::Utf8Value(isolate, json));
      header.emplace("content-type", "application/json");
    } else if (tmp->IsString()) {
      body = cpr::Body(*v8::String::Utf8Value(isolate, tmp));
    } else if (tmp->IsArrayBuffer()) {
      auto arraybuffer = tmp.As<v8::ArrayBuffer>();
      auto content = arraybuffer->GetContents();
      body = cpr::Body(static_cast<const char*>(content.Data()), content.ByteLength());
      header.emplace("content-type", "application/octet-stream");
    }
    if (body.size() != 0) {
      if (config->Get(context, NewV8Key(isolate, "deflate")).FromMaybe(undefined)->IsTrue()) {
        uLong dest_len = compressBound(body.size());
        char* dest = new char[dest_len];
        int rst = compress(reinterpret_cast<Bytef*>(dest), &dest_len, reinterpret_cast<const Bytef*>(body.data()),
                           body.size());
        if (rst != Z_OK) {
          delete[] dest;
          const char* msg;
          if (rst == Z_MEM_ERROR) {
            msg = "zlib error: there was not enough memory";
          } else if (rst == Z_BUF_ERROR) {
            msg = "zlib error: there was not enough room in the output buffer";
          } else {
            msg = "zlib error: unknown error";
          }
          resolver->Reject(context, v8::Exception::Error(NewV8String(isolate, msg))).IsJust();
          return;
        }
        body = cpr::Body(dest, dest_len);
        delete[] dest;
        header.emplace("content-encoding", "deflate");
      }
      session.SetBody(body);
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "maxRedirects")).FromMaybe(undefined);
    if (tmp->IsInt32()) {
      session.SetMaxRedirects(cpr::MaxRedirects(tmp->Int32Value(context).FromMaybe(3)));
    } else {
      session.SetMaxRedirects(cpr::MaxRedirects(3));
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "timeout")).FromMaybe(undefined);
    if (tmp->IsInt32()) {
      session.SetTimeout(tmp->Int32Value(context).FromMaybe(5000));
    } else {
      session.SetTimeout(5000);
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "connectTimeout")).FromMaybe(undefined);
    if (tmp->IsInt32()) {
      session.SetConnectTimeout(tmp->Int32Value(context).FromMaybe(1000));
    } else {
      session.SetConnectTimeout(1000);
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "headers")).FromMaybe(undefined);
    if (tmp->IsObject()) {
      auto object = tmp.As<v8::Object>();
      v8::Local<v8::Array> props;
      if (object->GetOwnPropertyNames(context).ToLocal(&props)) {
        for (int i = 0; i < props->Length(); i++) {
          v8::HandleScope handle_scope(isolate);
          v8::Local<v8::Value> key, val;
          if (props->Get(context, i).ToLocal(&key) && object->Get(context, key).ToLocal(&val)) {
            header.emplace(*v8::String::Utf8Value(isolate, key), *v8::String::Utf8Value(isolate, val));
          }
        }
      }
    }
  }
  session.SetHeader(header);
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

  auto ret_val = v8::Object::New(isolate);
  ret_val->Set(context, NewV8Key(isolate, "config"), config).IsJust();
  if (r.error.code == cpr::ErrorCode::OK) {
    auto headers = v8::Object::New(isolate);
    for (auto& h : r.header) {
      auto key = NewV8String(isolate, h.first.data(), h.first.size());
      auto val = NewV8String(isolate, h.second.data(), h.second.size());
      headers->Set(context, key, val).IsJust();
    }
    ret_val->Set(context, NewV8Key(isolate, "status"), v8::Int32::New(isolate, r.status_code)).IsJust();
    ret_val->Set(context, NewV8Key(isolate, "data"), NewV8String(isolate, r.text.data(), r.text.size())).IsJust();
    ret_val->Set(context, NewV8Key(isolate, "headers"), headers).IsJust();
    resolver->Resolve(context, ret_val).IsJust();
  } else {
    auto error = v8::Object::New(isolate);
    auto code = v8::Int32::New(isolate, static_cast<int32_t>(r.error.code));
    auto message = NewV8String(isolate, r.error.message.data(), r.error.message.size());
    error->Set(context, NewV8Key(isolate, "code"), code).IsJust();
    error->Set(context, NewV8Key(isolate, "message"), message).IsJust();
    ret_val->Set(context, NewV8Key(isolate, "error"), error).IsJust();
    resolver->Reject(context, ret_val).IsJust();
  }
}

}  // namespace openrasp_v8