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
	"reflect"
	"unsafe"
)

func underlyingString(s string) C.Buffer {
	p := (*reflect.StringHeader)(unsafe.Pointer(&s))
	return C.Buffer{data: unsafe.Pointer(p.Data), raw_size: C.size_t(p.Len)}
}

func underlyingBytes(b []byte) C.Buffer {
	p := (*reflect.SliceHeader)(unsafe.Pointer(&b))
	return C.Buffer{data: unsafe.Pointer(p.Data), raw_size: C.size_t(p.Len)}
}

func underlying(i interface{}) C.Buffer {
	switch t := i.(type) {
	case string:
		return underlyingString(t)
	case []byte:
		return underlyingBytes(t)
	default:
		return C.Buffer{nil, 0}
	}
}
