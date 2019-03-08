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

void openrasp::plugin_info(Isolate* isolate, const std::string& message) {
  pluginLog({message.c_str(), message.length()});
}
void openrasp::alarm_info(Isolate* isolate,
                          v8::Local<v8::String> type,
                          v8::Local<v8::Object> params,
                          v8::Local<v8::Object> result) {
  v8::Local<v8::Value> val;
  if (v8::JSON::Stringify(isolate->GetCurrentContext(), result).ToLocal(&val)) {
    v8::String::Utf8Value value(isolate, val);
    alarmLog({*value, static_cast<size_t>(value.length())});
  }
}