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

// ContextGetters request context top fields getter
type ContextGetters struct {
	Url         func() interface{}
	Path        func() interface{}
	Querystring func() interface{}
	Method      func() interface{}
	Protocol    func() interface{}
	RemoteAddr  func() interface{}
	Header      func() interface{}
	Parameter   func() interface{}
	Json        func() interface{}
	Server      func() interface{}
	AppBasePath func() interface{}
	Body        func() interface{}
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
	if context != nil && context.Url != nil {
		return C.CreateV8String(isolate, underlying(context.Url()))
	}
	return nil
}

//export pathGetter
func pathGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Path != nil {
		return C.CreateV8String(isolate, underlying(context.Path()))
	}
	return nil
}

//export querystringGetter
func querystringGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Querystring != nil {
		return C.CreateV8String(isolate, underlying(context.Querystring()))
	}
	return nil
}

//export methodGetter
func methodGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Method != nil {
		return C.CreateV8String(isolate, underlying(context.Method()))
	}
	return nil
}

//export protocolGetter
func protocolGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Protocol != nil {
		return C.CreateV8String(isolate, underlying(context.Protocol()))
	}
	return nil
}

//export remoteAddrGetter
func remoteAddrGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.RemoteAddr != nil {
		return C.CreateV8String(isolate, underlying(context.RemoteAddr()))
	}
	return nil
}

//export headerGetter
func headerGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Header != nil {
		return C.CreateV8String(isolate, underlying(context.Header()))
	}
	return nil
}

//export jsonGetter
func jsonGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Json != nil {
		return C.CreateV8String(isolate, underlying(context.Json()))
	}
	return nil
}

//export parameterGetter
func parameterGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Parameter != nil {
		return C.CreateV8String(isolate, underlying(context.Parameter()))
	}
	return nil
}

//export serverGetter
func serverGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Server != nil {
		return C.CreateV8String(isolate, underlying(context.Server()))
	}
	return nil
}

//export appBasePathGetter
func appBasePathGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.AppBasePath != nil {
		return C.CreateV8String(isolate, underlying(context.AppBasePath()))
	}
	return nil
}

//export bodyGetter
func bodyGetter(isolate unsafe.Pointer, index int) unsafe.Pointer {
	context := LookupContext(index)
	if context != nil && context.Body != nil {
		return C.CreateV8ArrayBuffer(isolate, underlying(context.Body()))
	}
	return nil
}
