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

#include "base/bundle.h"

namespace openrasp {
using openrasp_v8::Initialize;
using openrasp_v8::NewV8String;
using openrasp_v8::Platform;
using openrasp_v8::PluginFile;
using openrasp_v8::Snapshot;
class Isolate : public openrasp_v8::Isolate {
 public:
  static Isolate* New(Snapshot* snapshot_blob, uint64_t timestamp) {
    auto isolate = reinterpret_cast<Isolate*>(openrasp_v8::Isolate::New(snapshot_blob, timestamp));
    isolate->Enter();
    v8::HandleScope handle_scope(isolate);
    isolate->Initialize();
    isolate->GetData()->context.Get(isolate)->Enter();
    return isolate;
  }
  void Dispose() {
    Exit();
    openrasp_v8::Isolate::Dispose();
  }
};
}