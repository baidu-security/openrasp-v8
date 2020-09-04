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

#pragma once
#include <cpr/cpr.h>
#include <zlib.h>

#include <algorithm>
#include <string>

#include "bundle.h"

namespace openrasp_v8 {
// HTTP部分基于libcurl的c++接口cpr
class HTTPResponse : public cpr::Response {
 public:
  HTTPResponse() = default;
  HTTPResponse(const cpr::Response& response) : cpr::Response(response) {}
  v8::Local<v8::Object> ToObject(v8::Isolate* isolate);
};

class HTTPRequest : public cpr::Session {
 public:
  HTTPRequest() = default;
  HTTPRequest(v8::Isolate* isolate, v8::Local<v8::Value> conf);
  void SetMethod(const std::string& method) { this->method = method; }
  HTTPResponse GetResponse();
  std::string GetUrl() const;

 private:
  std::string method;
  std::string url;
  std::string error;
};

}  // namespace openrasp_v8