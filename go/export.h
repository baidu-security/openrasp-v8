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
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const void* data;
  size_t raw_size;
#ifdef __cplusplus
  const char* operator*() const { return reinterpret_cast<const char*>(data); }
  size_t length() const { return raw_size; }
#endif
} Buffer;

void* CreateV8String(void* isolate, Buffer buf);
void* CreateV8ArrayBuffer(void* isolate, Buffer buf);

char Initialize();
char Dispose();
char ClearPlugin();
char AddPlugin(Buffer source, Buffer name);
char CreateSnapshot(Buffer config);
Buffer Check(Buffer type, Buffer params, int context_index, int timeout);
Buffer ExecScript(Buffer source, Buffer name);

#ifdef __cplusplus
}
#endif
