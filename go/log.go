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

package v8

/*
#cgo CFLAGS: -std=c11
#cgo CXXFLAGS: -std=c++11
#cgo pkg-config: openrasp-v8.pc
#include "export.h"
#include "stdlib.h"
*/
import "C"

//PluginLogCB the plugin log callback, any content type, should output directly, should not append trailing '\n'
type PluginLogCB func(str string)

var pluginLogCB PluginLogCB

//export pluginLog
func pluginLog(buf C.Buffer) {
	if pluginLogCB != nil {
		pluginLogCB(C.GoStringN((*C.char)(buf.data), C.int(buf.raw_size)))
	}
}
