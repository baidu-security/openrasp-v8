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

// ContextGetters request context top fields getter
type ContextGetters struct {
	url         func() interface{}
	path        func() interface{}
	querystring func() interface{}
	method      func() interface{}
	protocol    func() interface{}
	remoteAddr  func() interface{}
	header      func() interface{}
	parameter   func() interface{}
	json        func() interface{}
	server      func() interface{}
	appBasePath func() interface{}
	body        func() interface{}
}

//export urlGetter
func urlGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.url != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.url()))
	}
}

//export pathGetter
func pathGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.path != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.path()))
	}
}

//export querystringGetter
func querystringGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.querystring != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.querystring()))
	}
}

//export methodGetter
func methodGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.method != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.method()))
	}
}

//export protocolGetter
func protocolGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.protocol != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.protocol()))
	}
}

//export remoteAddrGetter
func remoteAddrGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.remoteAddr != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.remoteAddr()))
	}
}

//export headerGetter
func headerGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.header != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.header()))
	}
}

//export jsonGetter
func jsonGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.json != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.json()))
	}
}

//export parameterGetter
func parameterGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.parameter != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.parameter()))
	}
}

//export serverGetter
func serverGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.server != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.server()))
	}
}

//export appBasePathGetter
func appBasePathGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.appBasePath != nil {
		C.CreateV8String(isolate, maybe, underlying(getters.appBasePath()))
	}
}

//export bodyGetter
func bodyGetter(isolate unsafe.Pointer, maybe unsafe.Pointer) {
	getters := *(*ContextGetters)(C.GetContextGetters(isolate))
	if getters.body != nil {
		C.CreateV8ArrayBuffer(isolate, maybe, underlying(getters.body()))
	}
}
