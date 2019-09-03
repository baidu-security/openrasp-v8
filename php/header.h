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

bool ExtractBuildinAction(openrasp_v8::Isolate* isolate, std::map<std::string, std::string>& buildin_action_map);
bool ExtractCallableBlacklist(openrasp_v8::Isolate* isolate, std::vector<std::string>& callable_blacklist);
bool ExtractXSSConfig(openrasp_v8::Isolate* isolate,
                      std::string& filter_regex,
                      int64_t& min_length,
                      int64_t& max_detection_num);