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
#cgo pkg-config: v8.pc
#include "export.h"
#include "stdlib.h"
*/
import "C"
import (
	"unsafe"
)

//Plugin plugin source and filename
type Plugin struct {
	source   string
	filename string
}

//Initialize initialize V8
func Initialize(cb1 PluginLogCB, cb2 AlarmLogCB) bool {
	pluginLogCB = cb1
	alarmLogCB = cb2
	return C.Initialize() != 0
}

//CreateSnapshot create snapshot with config and plugins
func CreateSnapshot(config string, plugins []Plugin) bool {
	C.ClearPlugin()
	for _, plugin := range plugins {
		C.AddPlugin(underlyingString(plugin.source), underlyingString(plugin.filename))
	}
	return C.CreateSnapshot(underlyingString(config)) != 0
}

//Check check request
func Check(requestType string, requestParams []byte, requestContextGetters ContextGetters) {
	C.Check(underlyingString(requestType), underlyingBytes(requestParams), unsafe.Pointer(&requestContextGetters))
}

//ExecScript execute any script
func ExecScript(source string, filename string) string {
	buf := C.ExecScript(underlyingString(source), underlyingString(filename))
	return C.GoStringN((*C.char)(buf.data), C.int(buf.raw_size))
}
