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

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/assert"
)

// func TestMain(m *testing.M) {
// 	rst := m.Run()
// 	if rst != 0 {
// 		os.Exit(rst)
// 	}
// 	if !Dispose() {
// 		os.Exit(1)
// 	}
// }

func TestInitilize(t *testing.T) {
	rst := Initialize(nil)
	assert.True(t, rst)
}

func TestCreateSnapshot(t *testing.T) {
	Initialize(func(s string) {
		assert.Equal(t, s, "[test] ok\n")
	})
	rst := CreateSnapshot("const ok = 'ok'", []Plugin{
		Plugin{
			Source: `const plugin = new RASP('test')
			plugin.register('request', () => {})
			plugin.log(ok)`,
			Filename: "plugin.js",
		},
	})
	assert.True(t, rst)
}

func TestExecScript(t *testing.T) {
	Initialize(nil)
	CreateSnapshot("", []Plugin{})
	rst := ExecScript(`(function() {
		return {
			rst: 1 + 1
		}
	})()`, "test.js")
	assert.Equal(t, rst, `{"rst":2}`)
}

func TestCheck(t *testing.T) {
	Initialize(nil)
	CreateSnapshot("", []Plugin{
		Plugin{
			Source: `const plugin = new RASP('test1')
			plugin.register('request', () => {
				return {
					action: 'ignore'
				}
			})`,
			Filename: "plugin1.js",
		},
		Plugin{
			Source: `const plugin = new RASP('test2')
			plugin.register('command', () => {
				return {
					action: 'block'
				}
			})`,
			Filename: "plugin2.js",
		},
		Plugin{
			Source: `const plugin = new RASP('test3')
			plugin.register('directory', () => {
				for(;;){}
			})`,
			Filename: "plugin3.js",
		},
	})
	params := []byte("{}")
	contextGetters := &ContextGetters{}
	rst := Check("request", params, contextGetters, 100)
	assert.Equal(t, string(rst), ``)

	rst = Check("command", params, contextGetters, 100)
	assert.Equal(t, string(rst), `[{"action":"block","message":"","name":"test2","confidence":0}]`)

	rst = Check("directory", params, contextGetters, 100)
	assert.Equal(t, string(rst), `[{"action":"log","message":"Javascript plugin execution timeout"}]`)
}

func TestPluginLog(t *testing.T) {
	Initialize(func(s string) {
		assert.Equal(t, s, "2333\n")
	})
	CreateSnapshot("", []Plugin{})
	ExecScript("console.log(2333)", "test.js")
}

func TestParams(t *testing.T) {
	jsonStr := `{"a":1,"b":"2","c":[1,"2"]}`
	Initialize(func(s string) {
		assert.Equal(t, s, jsonStr+"\n")
	})
	CreateSnapshot("", []Plugin{
		Plugin{
			Source: `const plugin = new RASP('test')
			plugin.register('request', (params, context) => {
				console.log(JSON.stringify(params))
				return {
					action: 'ignore'
				}
			})`,
			Filename: "plugin1.js",
		},
	})
	params := []byte(jsonStr)
	contextGetters := &ContextGetters{}
	rst := Check("request", params, contextGetters, 100)
	assert.Equal(t, string(rst), ``)
}

func TestContext(t *testing.T) {
	jsonStr := `{"json":{"q":2},"server":{"name":"go"},"body":{},"appBasePath":"/","remoteAddr":"127.0.0.1","protocol":"https","method":"post","querystring":"p=1","path":"/doc/dev/api/context.html?p=1","parameter":{"p":["1"],"q":[2]},"header":{"user-agent":"Chrome"},"url":"https://rasp.baidu.com/doc/dev/api/context.html?p=1"}`
	context := map[string]interface{}{}
	json.Unmarshal([]byte(jsonStr), &context)
	contextGetters := &ContextGetters{
		Url:         func() interface{} { return context["url"] },
		Path:        func() interface{} { return context["path"] },
		Querystring: func() interface{} { return context["querystring"] },
		Method:      func() interface{} { return context["method"] },
		Protocol:    func() interface{} { return context["protocol"] },
		RemoteAddr:  func() interface{} { return context["remoteAddr"] },
		AppBasePath: func() interface{} { return context["appBasePath"] },
		Body:        func() interface{} { return context["body"] },
		Header: func() interface{} {
			ret, _ := json.Marshal(context["header"])
			return ret
		},
		Parameter: func() interface{} {
			ret, _ := json.Marshal(context["parameter"])
			return ret
		},
		Server: func() interface{} {
			ret, _ := json.Marshal(context["server"])
			return ret
		},
		Json: func() interface{} {
			ret, _ := json.Marshal(context["json"])
			return ret
		},
	}
	Initialize(func(s string) {
		assert.Equal(t, s, jsonStr+"\n")
	})
	CreateSnapshot("", []Plugin{
		Plugin{
			Source: `const plugin = new RASP('test')
			plugin.register('request', (params, context) => {
				console.log(JSON.stringify(context))
				return {
					action: 'ignore'
				}
			})`,
			Filename: "plugin1.js",
		},
	})
	params := []byte("{}")
	rst := Check("request", params, contextGetters, 100)
	assert.Equal(t, string(rst), ``)
}
