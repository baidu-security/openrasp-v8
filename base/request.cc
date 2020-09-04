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

#include "request.h"

#include <string>

#include "bundle.h"
#include "thread-pool.h"

namespace openrasp_v8 {

v8::Local<v8::Object> HTTPResponse::ToObject(v8::Isolate* isolate) {
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
  auto object = v8::Object::New(isolate);
  if (!error) {
    auto headers = v8::Object::New(isolate);
    for (auto& h : header) {
      auto key = NewV8String(isolate, h.first.data(), h.first.size());
      auto val = NewV8String(isolate, h.second.data(), h.second.size());
      headers->Set(context, key, val).IsJust();
    }
    object->Set(context, NewV8Key(isolate, "status"), v8::Int32::New(isolate, status_code)).IsJust();
    object->Set(context, NewV8Key(isolate, "data"), NewV8String(isolate, text.data(), text.size())).IsJust();
    object->Set(context, NewV8Key(isolate, "headers"), headers).IsJust();
  } else {
    auto e = v8::Object::New(isolate);
    auto code = v8::Int32::New(isolate, static_cast<int32_t>(error.code));
    auto message = NewV8String(isolate, error.message.data(), error.message.size());
    e->Set(context, NewV8Key(isolate, "code"), code).IsJust();
    e->Set(context, NewV8Key(isolate, "message"), message).IsJust();
    object->Set(context, NewV8Key(isolate, "error"), e).IsJust();
  }
  return object;
}

HTTPRequest::HTTPRequest(v8::Isolate* isolate, v8::Local<v8::Value> conf) {
  v8::HandleScope handle_scope(isolate);
  v8::TryCatch try_catch(isolate);
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::Object> config;
  if (!conf->ToObject(context).ToLocal(&config)) {
    auto message = try_catch.Message();
    if (!message.IsEmpty()) {
      error = *v8::String::Utf8Value(isolate, message->Get());
    }
    return;
  }
  auto undefined = v8::Undefined(isolate).As<v8::Value>();
  cpr::Header header;
  cpr::Parameters parameters;
  cpr::Body body;
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
      url = *v8::String::Utf8Value(isolate, tmp);
      SetUrl(url);
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
        SetParameters(std::move(parameters));
      }
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "data")).FromMaybe(undefined);
    if (tmp->IsObject()) {
      v8::Local<v8::Value> json;
      if (!v8::JSON::Stringify(context, tmp).ToLocal(&json)) {
        auto message = try_catch.Message();
        if (!message.IsEmpty()) {
          error = *v8::String::Utf8Value(isolate, message->Get());
        }
        return;
      }
      body = cpr::Body(*v8::String::Utf8Value(isolate, json));
      header.emplace("content-type", "application/json");
    } else if (tmp->IsString()) {
      body = cpr::Body(*v8::String::Utf8Value(isolate, tmp));
      // } else if (tmp->IsArrayBuffer()) {
      //   auto arraybuffer = tmp.As<v8::ArrayBuffer>();
      //   auto content = arraybuffer->GetContents();
      //   body = cpr::Body(static_cast<const char*>(content.Data()), content.ByteLength());
      //   header.emplace("content-type", "application/octet-stream");
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
          error = msg;
          return;
        }
        body = cpr::Body(dest, dest_len);
        delete[] dest;
        header.emplace("content-encoding", "deflate");
      }
      SetBody(std::move(body));
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "maxRedirects")).FromMaybe(undefined);
    if (tmp->IsInt32()) {
      SetMaxRedirects(cpr::MaxRedirects(tmp->Int32Value(context).FromMaybe(3)));
    } else {
      SetMaxRedirects(cpr::MaxRedirects(3));
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "timeout")).FromMaybe(undefined);
    if (tmp->IsInt32()) {
      SetTimeout(tmp->Int32Value(context).FromMaybe(5000));
    } else {
      SetTimeout(5000);
    }
  }
  {
    v8::HandleScope handle_scope(isolate);
    auto tmp = config->Get(context, NewV8Key(isolate, "connectTimeout")).FromMaybe(undefined);
    if (tmp->IsInt32()) {
      SetConnectTimeout(tmp->Int32Value(context).FromMaybe(5000));
    } else {
      SetConnectTimeout(5000);
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
  SetHeader(header);
  SetVerifySsl(false);
}

HTTPResponse HTTPRequest::GetResponse() {
  if (!error.empty()) {
    cpr::Error e;
    e.code = cpr::ErrorCode::UNKNOWN_ERROR;
    e.message = error;
    HTTPResponse r;
    r.error = e;
    return r;
  } else if (method == "get") {
    return Get();
  } else if (method == "post") {
    return Post();
  } else if (method == "put") {
    return Put();
  } else if (method == "patch") {
    return Patch();
  } else if (method == "head") {
    return Head();
  } else if (method == "options") {
    return Options();
  } else if (method == "delete") {
    return Delete();
  } else {
    return Get();
  }
}

std::string HTTPRequest::GetUrl() const {
  return this->url;
}

size_t AsyncRequest::pool_size = 1;
size_t AsyncRequest::queue_cap = 1;

AsyncRequest::AsyncRequest(std::shared_ptr<ThreadPool> pool) : pool(pool) {}

bool AsyncRequest::Submit(std::shared_ptr<HTTPRequest> request) {
  return pool->Post([request]() {
    auto response = request->GetResponse();
    if (response.error) {
      Platform::logger(std::string("async request failed; url: ") + request->GetUrl() +
                       ", errMsg: " + response.error.message + std::string("\n"));
    } else if (response.status_code != 200) {
      Platform::logger(std::string("async request status: ") + std::to_string(response.status_code) + ", url: " +
                       request->GetUrl() + ", body: " + response.text.substr(0, 1024) + std::string("\n"));
    }
  });
}

size_t AsyncRequest::GetQueueSize() {
  return pool->GetQueueSize();
}

void AsyncRequest::ConfigInstance(size_t pool_size, size_t queue_cap) {
  AsyncRequest::pool_size = pool_size;
  AsyncRequest::queue_cap = queue_cap;
}

AsyncRequest& AsyncRequest::GetInstance() {
  static AsyncRequest instance(std::make_shared<ThreadPool>(pool_size, queue_cap));
  if (!instance.pool) {
    instance = AsyncRequest(std::make_shared<ThreadPool>(pool_size, queue_cap));
  }
  return instance;
}

void AsyncRequest::Terminate() {
  GetInstance().pool.reset();
}

}  // namespace openrasp_v8