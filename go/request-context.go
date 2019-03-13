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
	"sync"
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

var mu sync.Mutex
var index int
var contexts = make(map[int]*ContextGetters)

// RegisterContext register a context to contexts map and get an index
func RegisterContext(context *ContextGetters) int {
	mu.Lock()
	defer mu.Unlock()
	index++
	for contexts[index] != nil || index == 0 {
		index++
	}
	contexts[index] = context
	return index
}

// LookupContext lookup a context by index
func LookupContext(i int) *ContextGetters {
	mu.Lock()
	defer mu.Unlock()
	return contexts[i]
}

// UnregisterContext unregister a context by index
func UnregisterContext(i int) {
	mu.Lock()
	defer mu.Unlock()
	delete(contexts, i)
}

//export urlGetter
func urlGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.url != nil {
		return C.CreateV8String(isolate, underlying(context.url()))
	}
	return nil
}

//export pathGetter
func pathGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.path != nil {
		return C.CreateV8String(isolate, underlying(context.path()))
	}
	return nil
}

//export querystringGetter
func querystringGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.querystring != nil {
		return C.CreateV8String(isolate, underlying(context.querystring()))
	}
	return nil
}

//export methodGetter
func methodGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.method != nil {
		return C.CreateV8String(isolate, underlying(context.method()))
	}
	return nil
}

//export protocolGetter
func protocolGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.protocol != nil {
		return C.CreateV8String(isolate, underlying(context.protocol()))
	}
	return nil
}

//export remoteAddrGetter
func remoteAddrGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.remoteAddr != nil {
		return C.CreateV8String(isolate, underlying(context.remoteAddr()))
	}
	return nil
}

//export headerGetter
func headerGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.header != nil {
		return C.CreateV8String(isolate, underlying(context.header()))
	}
	return nil
}

//export jsonGetter
func jsonGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.json != nil {
		return C.CreateV8String(isolate, underlying(context.json()))
	}
	return nil
}

//export parameterGetter
func parameterGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.parameter != nil {
		return C.CreateV8String(isolate, underlying(context.parameter()))
	}
	return nil
}

//export serverGetter
func serverGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.server != nil {
		return C.CreateV8String(isolate, underlying(context.server()))
	}
	return nil
}

//export appBasePathGetter
func appBasePathGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.appBasePath != nil {
		return C.CreateV8String(isolate, underlying(context.appBasePath()))
	}
	return nil
}

//export bodyGetter
func bodyGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.body != nil {
		return C.CreateV8ArrayBuffer(isolate, underlying(context.body()))
	}
	return nil
}
