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
import (
	"sync"
	"unsafe"
)

//Plugin plugin source and filename
type Plugin struct {
	Source   string
	Filename string
}

var rw sync.RWMutex

//Initialize initialize V8
func Initialize(cb PluginLogCB) bool {
	rw.Lock()
	defer rw.Unlock()
	pluginLogCB = cb
	return C.Initialize() != 0
}

//Dispose dispose V8, can not reinitialze
func Dispose() bool {
	rw.Lock()
	defer rw.Unlock()
	return C.Dispose() != 0
}

//CreateSnapshot create snapshot with config and plugins
func CreateSnapshot(config string, plugins []Plugin) bool {
	rw.Lock()
	defer rw.Unlock()
	C.ClearPlugin()
	for _, plugin := range plugins {
		C.AddPlugin(underlyingString(plugin.Source), underlyingString(plugin.Filename))
	}
	return C.CreateSnapshot(underlyingString(config)) != 0
}

//Check check request
func Check(requestType string, requestParams []byte, requestContext *ContextGetters, timeout int) []byte {
	rw.RLock()
	defer rw.RUnlock()
	contextIndex := RegisterContext(requestContext)
	defer UnregisterContext(contextIndex)
	buf := C.Check(underlyingString(requestType), underlyingBytes(requestParams), C.int(contextIndex), C.int(timeout))
	defer C.free(unsafe.Pointer(buf.data))
	return C.GoBytes(unsafe.Pointer(buf.data), C.int(buf.raw_size))
}

//ExecScript execute any script
func ExecScript(source string, filename string) string {
	rw.RLock()
	defer rw.RUnlock()
	buf := C.ExecScript(underlyingString(source), underlyingString(filename))
	return C.GoStringN((*C.char)(buf.data), C.int(buf.raw_size))
}
