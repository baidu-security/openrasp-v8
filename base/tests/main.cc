#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "../bundle.h"
#include "../request.h"
#include "../thread-pool.h"
#include "catch2/catch.hpp"

using namespace openrasp_v8;

std::mutex mtx;
std::string message;
void plugin_log(const std::string& msg) {
  message = msg;
  // std::cout << msg << std::endl;
}

class IsolateDeleter {
 public:
  void operator()(openrasp_v8::Isolate* isolate) { isolate->Dispose(); }
};
typedef std::unique_ptr<openrasp_v8::Isolate, IsolateDeleter> IsolatePtr;

struct Listener : Catch::TestEventListenerBase {
  using TestEventListenerBase::TestEventListenerBase;  // inherit constructor
  void testRunStarting(Catch::TestRunInfo const& testRunInfo) override { Initialize(0, plugin_log); }
  void testRunEnded(Catch::TestRunStats const& testRunStats) override { Dispose(); }
};
CATCH_REGISTER_LISTENER(Listener);

TEST_CASE("Bench", "[!benchmark]") {
  Snapshot snapshot("", {{"test", R"(
        const plugin = new RASP('test')
        plugin.register('request', params => params)
    )"}},
                    "1.2.3", 100);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();

  BENCHMARK("ignore", i) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
    v8::Context::Scope context_scope(v8_context);
    auto type = NewV8String(isolate, "request");
    auto json = NewV8String(isolate, R"({"action":"ignore","message":"1234567890","name":"test","confidence":0})");
    auto params = v8::JSON::Parse(v8_context, json).ToLocalChecked().As<v8::Object>();
    auto context = v8::Object::New(isolate);
    context->Set(v8_context, NewV8Key(isolate, "index"), v8::Int32::New(isolate, i)).IsJust();
    auto rst = isolate->Check(type, params, context, 100);
    auto n = rst->Length();
    REQUIRE(n == 0);
    return n;
  };

  {
    v8::HeapStatistics stat;
    isolate->GetHeapStatistics(&stat);
    printf("\nv8 heap statistics\n");
    printf("total_physical_size: %zu (KB)\n", stat.total_physical_size() / 1024);
    printf("total_heap_size:     %zu (KB)\n", stat.total_heap_size() / 1024);
    printf("used_heap_size:      %zu (KB)\n", stat.used_heap_size() / 1024);
  }

  BENCHMARK("log", i) {
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
    v8::Context::Scope context_scope(v8_context);
    auto type = NewV8String(isolate, "request");
    auto json = NewV8String(isolate, R"({"action":"log","message":"1234567890","name":"test","confidence":0})");
    auto params = v8::JSON::Parse(v8_context, json).ToLocalChecked().As<v8::Object>();
    auto context = v8::Object::New(isolate);
    context->Set(v8_context, NewV8Key(isolate, "index"), v8::Int32::New(isolate, i)).IsJust();
    auto rst = isolate->Check(type, params, context, 100);
    auto n = rst->Length();
    REQUIRE(n == 1);
    return n;
  };

  {
    v8::HeapStatistics stat;
    isolate->GetHeapStatistics(&stat);
    printf("\nv8 heap statistics\n");
    printf("total_physical_size: %zu (KB)\n", stat.total_physical_size() / 1024);
    printf("total_heap_size:     %zu (KB)\n", stat.total_heap_size() / 1024);
    printf("used_heap_size:      %zu (KB)\n", stat.used_heap_size() / 1024);
  }
}
class DummyTask : public v8::Task {
 public:
  void Run() override {}
};
TEST_CASE("Platform") {
  Platform::Get()->Startup();
  Platform::Get()->Startup();

  Platform::Get()->Shutdown();
  Platform::Get()->Shutdown();

  Platform::Get()->Startup();
  Platform::Get()->Shutdown();

  Platform::Get()->Shutdown();
  Platform::Get()->Startup();

  Platform::Get()->Startup();

  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  Platform::Get()->PumpMessageLoop(isolate);
  Platform::Get()->CallOnForegroundThread(isolate, new DummyTask());
  Platform::Get()->CallDelayedOnForegroundThread(isolate, new DummyTask(), 0);
  Platform::Get()->CallOnWorkerThread(std::unique_ptr<DummyTask>(new DummyTask()));
  Platform::Get()->CallDelayedOnWorkerThread(std::unique_ptr<DummyTask>(new DummyTask()), 0);
  Platform::Get()->CurrentClockTimeMillis();
  isolate->Dispose();
}

TEST_CASE("Snapshot") {
  SECTION("Constructor 1") {
    {
      Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000, nullptr);
      REQUIRE(snapshot.data != nullptr);
      REQUIRE(snapshot.raw_size > 0);
      REQUIRE(snapshot.timestamp == 1000);
    }
    {
      Snapshot snapshot("wrong syntax", std::vector<PluginFile>(), "1.2.3", 1000, nullptr);
      REQUIRE(snapshot.data != nullptr);
      REQUIRE(snapshot.raw_size > 0);
      REQUIRE(snapshot.timestamp == 1000);
    }
    {
      Snapshot snapshot("", {{"wrong-syntax", "wrong syntax"}}, "1.2.3", 1000, nullptr);
      REQUIRE(snapshot.data != nullptr);
      REQUIRE(snapshot.raw_size > 0);
      REQUIRE(snapshot.timestamp == 1000);
    }
    {
      Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000, nullptr);
      auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
      REQUIRE(isolate != nullptr);
      IsolatePtr ptr(isolate);
      isolate->Initialize();
      v8::Isolate::Scope isolate_scope(isolate);
      v8::HandleScope handle_scope(isolate);
      v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
      v8::Context::Scope context_scope(v8_context);
      auto maybe_rst = isolate->ExecScript("RASP.get_version()", "version");
      v8::String::Utf8Value rst(isolate, maybe_rst.ToLocalChecked());
      REQUIRE(std::string(*rst) == "1.2.3");
    }
  }

  SECTION("Constructor 2") {
    auto data = new char[10];
    auto raw_size = 10;
    auto timestamp = 1000;
    Snapshot snapshot(data, raw_size, timestamp);
    REQUIRE(snapshot.data == data);
    REQUIRE(snapshot.raw_size == raw_size);
    REQUIRE(snapshot.timestamp == timestamp);
  }

  SECTION("Constructor 3") {
    Snapshot snapshot1("", std::vector<PluginFile>(), "1.2.3", 1000, nullptr);
    snapshot1.Save("openrasp-v8-base-tests-snapshot");
    Snapshot snapshot2("openrasp-v8-base-tests-snapshot", 1000);
    REQUIRE(snapshot1.raw_size == snapshot2.raw_size);
    REQUIRE(memcmp(snapshot1.data, snapshot2.data, snapshot1.raw_size) == 0);
  }

  SECTION("IsOK") {
    {
      Snapshot snapshot(nullptr, 1, 0);
      REQUIRE_FALSE(snapshot.IsOk());
    }
    {
      Snapshot snapshot(new char[10], 0, 0);
      REQUIRE_FALSE(snapshot.IsOk());
    }
    {
      Snapshot snapshot(new char[10], 10, 0);
      REQUIRE(snapshot.IsOk());
    }
  }

  SECTION("IsExpired") {
    Snapshot snapshot(new char[10], 10, 1000);
    REQUIRE(snapshot.IsExpired(snapshot.timestamp + 1));
    REQUIRE_FALSE(snapshot.IsExpired(snapshot.timestamp - 1));
    REQUIRE_FALSE(snapshot.IsExpired(snapshot.timestamp));
  }

  SECTION("Save") {
    Snapshot snapshot(new char[4]{0, 1, 2, 3}, 4, 1000);
    REQUIRE_FALSE(snapshot.Save("not-exist/not-exist/not-exist"));
  }
}

TEST_CASE("Isolate") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000, nullptr);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);

  SECTION("GetData") { REQUIRE(isolate->GetData() != nullptr); }

  SECTION("SetData") {
    auto data = isolate->GetData();
    isolate->SetData(data);
    REQUIRE(isolate->GetData() == data);
  }

  SECTION("IsExpired") {
    REQUIRE(isolate->IsExpired(snapshot.timestamp + 1));
    REQUIRE_FALSE(isolate->IsExpired(snapshot.timestamp - 1));
    REQUIRE_FALSE(isolate->IsExpired(snapshot.timestamp));
  }

  SECTION("IsDead") { REQUIRE(!isolate->IsDead()); }

  SECTION("Check") {
    auto type = NewV8String(isolate, "request");
    auto params = v8::Object::New(isolate);
    auto context = v8::Object::New(isolate);
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 0);
  }

  SECTION("ExecScript") {
    {
      v8::TryCatch try_catch(isolate);
      auto maybe_rst = isolate->ExecScript("wrong syntax", "wrong-syntax");
      REQUIRE(maybe_rst.IsEmpty());
      REQUIRE(try_catch.HasCaught());
    }
    {
      auto maybe_rst = isolate->ExecScript("1+1", "test");
      REQUIRE_FALSE(maybe_rst.IsEmpty());
      REQUIRE(maybe_rst.ToLocalChecked()->IsNumber());
      REQUIRE(maybe_rst.ToLocalChecked()->NumberValue(isolate->GetCurrentContext()).ToChecked() == 2);
    }
  }
}

TEST_CASE("Exception") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);

  SECTION("NORMAL") {
    v8::TryCatch try_catch(isolate);
    isolate->ExecScript("throw new Error('2333')", "2333.js");
    REQUIRE(try_catch.HasCaught());
    Exception e(isolate, try_catch);
    REQUIRE(e == R"(2333.js:1
throw new Error('2333')
Error: 2333
    at 2333.js:1:7
)");
  }

  SECTION("TRUNCATED") {
    v8::TryCatch try_catch(isolate);
    isolate->ExecScript(std::string("JSON.parse('begin").append(5 * 1024, 'a').append("end')"), "2333.js");
    REQUIRE(try_catch.HasCaught());
    Exception e(isolate, try_catch);
    REQUIRE_THAT(e, !Catch::Matchers::Contains("end"));
  }

  SECTION("NON") {
    v8::TryCatch try_catch(isolate);
    Exception e(isolate, try_catch);
    REQUIRE(e == "");
  }

  SECTION("Native Error") {
    v8::MaybeLocal<v8::Value>().ToLocalChecked();
    REQUIRE_THAT(message, Catch::Matchers::Contains("Native error"));
  }
}

TEST_CASE("TimeoutTask") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);

  SECTION("Not Timeout") {
    auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    std::promise<void> pro;
    Platform::Get()->CallOnWorkerThread(std::unique_ptr<v8::Task>(new TimeoutTask(isolate, pro.get_future(), 1000)));
    isolate->ExecScript("for(let i=0;i<10;i++);", "loop");
    pro.set_value();
    auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    REQUIRE(end - start <= 1000);
  }

  SECTION("Timeout") {
    auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    std::promise<void> pro;
    Platform::Get()->CallOnWorkerThread(std::unique_ptr<v8::Task>(new TimeoutTask(isolate, pro.get_future(), 100)));
    isolate->ExecScript("for(;;);", "loop");
    pro.set_value();
    auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    REQUIRE(end - start >= 100);
  }
}

TEST_CASE("Flex") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);
  auto maybe_rst = isolate->ExecScript("JSON.stringify(RASP.sql_tokenize('a bb       ccc dddd   '))", "flex");
  REQUIRE_FALSE(maybe_rst.IsEmpty());
  REQUIRE(maybe_rst.ToLocalChecked()->IsString());
  REQUIRE(
      std::string(*v8::String::Utf8Value(isolate, maybe_rst.ToLocalChecked())) ==
      R"([{"start":0,"stop":1,"text":"a"},{"start":2,"stop":4,"text":"bb"},{"start":11,"stop":14,"text":"ccc"},{"start":15,"stop":19,"text":"dddd"}])");
}

TEST_CASE("Log") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);
  isolate->ExecScript("console.log(2333)", "log");
  REQUIRE(message == "2333\n");
  isolate->Log(v8::Object::New(isolate));
  REQUIRE(message == "{}\n");
}

TEST_CASE("Request", "[!mayfail]") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);
  SECTION("get") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request({
    method: 'get',
    url: 'https://www.httpbin.org/get',
    params: {
        a: 2333,
        b: '6666'
    },
    headers: {
        a: 2333,
        b: '6666'
    },
    data: {
        a: 2333,
        b: '6666'
    }
}).then(ret => {
  ret.data = JSON.parse(ret.data)
  return JSON.stringify(ret)
})
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kFulfilled);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("args":{"a":"2333","b":"6666"})==="));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("A":"2333")==="));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("B":"6666")==="));
  }
  SECTION("post") {
    SECTION("form") {
      auto maybe_rst = isolate->ExecScript(
          R"(
RASP.request({
    method: 'post',
    url: 'https://www.httpbin.org/post',
    params: {
        a: 2333,
        b: '6666'
    },
    headers: {
        a: 2333,
        b: '6666'
    },
    data: 'a=2333&b=6666'
}).then(ret => {
  ret.data = JSON.parse(ret.data)
  return JSON.stringify(ret)
})
          )",
          "request");
      auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
      REQUIRE(promise->State() == v8::Promise::PromiseState::kFulfilled);
      auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("args":{"a":"2333","b":"6666"})==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("A":"2333")==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("B":"6666")==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("form":{"a":"2333","b":"6666"})==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("Content-Type":"application/x-www-form-urlencoded")==="));
    }
    SECTION("json") {
      auto maybe_rst = isolate->ExecScript(
          R"(
RASP.request({
    method: 'post',
    url: 'https://www.httpbin.org/post',
    params: {
        a: 2333,
        b: '6666'
    },
    headers: {
        a: 2333,
        b: '6666',
        'content-type': 'application/json'
    },
    data: {
        a: 2333,
        b: '6666'
    }
}).then(ret => {
  ret.data = JSON.parse(ret.data)
  return JSON.stringify(ret)
})
          )",
          "request");
      auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
      REQUIRE(promise->State() == v8::Promise::PromiseState::kFulfilled);
      auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("args":{"a":"2333","b":"6666"})==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("A":"2333")==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("B":"6666")==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("json":{"a":2333,"b":"6666"})==="));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("Content-Type":"application/json")==="));
    }
    SECTION("deflate") {
      auto maybe_rst = isolate->ExecScript(
          R"(
RASP.request({
    method: 'post',
    url: 'https://www.httpbin.org/post',
    data: { a: 1 },
    deflate: true
}).then(ret => {
  ret.data = JSON.parse(ret.data)
  return JSON.stringify(ret)
})
          )",
          "request");
      auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
      REQUIRE(promise->State() == v8::Promise::PromiseState::kFulfilled);
      auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
      REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===(eJyrVkpUsjKsBQAIKgIJ)==="));
    }
  }
  SECTION("timeout") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request({
    url: 'https://www.httpbin.org/get',
    timeout: 10,
    connectTimeout: 1000
}).catch(err => Promise.reject(JSON.stringify(err)))
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kRejected);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE_THAT(rst, Catch::Matchers::Matches(R"===(.*timed out.*)==="));
  }
  SECTION("error") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request({
    url: 'https://www.httpbin.asdasdasdasdasdas'
}).catch(err => Promise.reject(JSON.stringify(err)))
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kRejected);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("code":3)==="));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===(Could not resolve host)==="));
  }
  SECTION("redirect") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request({
    url: 'https://httpbin.org/redirect/2',
    maxRedirects: 0
}).then(ret => JSON.stringify(ret))
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kFulfilled);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("/relative-redirect/1")==="));
  }
  SECTION("undefined config") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request().catch(err => err.error.message)
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kFulfilled);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE(rst == "Uncaught TypeError: Cannot convert undefined or null to object");
  }
  SECTION("empty config") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request({}).catch(err => Promise.reject(JSON.stringify(err)))
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kRejected);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("code":5)==="));
  }
  SECTION("other method") {
    isolate->ExecScript(
        R"(
RASP.request({method: 'put', url: 'https://www.httpbin.org/put', timeout: 10})
RASP.request({method: 'head', url: 'https://www.httpbin.org/head', timeout: 10})
RASP.request({method: 'patch', url: 'https://www.httpbin.org/patch', timeout: 10})
RASP.request({method: 'delete', url: 'https://www.httpbin.org/delete', timeout: 10})
RASP.request({method: 'options', url: 'https://www.httpbin.org/options', timeout: 10})
          )",
        "request");
  }
}

TEST_CASE("Check") {
  Snapshot snapshot("", {{"test", R"(
        const plugin = new RASP('test')
        plugin.register('request', (params) => {
            if (params.case === 'throw') { a.a() }
            if (params.case === 'timeout') { for(;;) {} }
            if (params.case === 'promise resolve') { delete params.case; return Promise.resolve(params) }
            if (params.case === 'promise reject') { delete params.case; return Promise.reject(params) }
            return params
        })
    )"}},
                    "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);
  auto type = NewV8String(isolate, "request");
  auto params = v8::Object::New(isolate);
  auto context = v8::Object::New(isolate);

  SECTION("ignore") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "ignore")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 0);
  }

  SECTION("log") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "log")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(std::string(*v8::String::Utf8Value(
                isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
            R"([{"action":"log","message":"","name":"test","confidence":0}])");
  }

  SECTION("block") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "block")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(std::string(*v8::String::Utf8Value(
                isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
            R"([{"action":"block","message":"","name":"test","confidence":0}])");
  }

  SECTION("timeout") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "log")).IsJust();
    params->Set(v8_context, NewV8Key(isolate, "case"), NewV8String(isolate, "timeout")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 0);
    REQUIRE(message == "Javascript plugin execution timeout\n");
  }

  SECTION("throw") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "log")).IsJust();
    params->Set(v8_context, NewV8Key(isolate, "case"), NewV8String(isolate, "throw")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 0);
    REQUIRE_THAT(message, Catch::Matchers::Contains("ReferenceError: a is not defined"));
  }

  SECTION("promise resolve ignore") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "ignore")).IsJust();
    params->Set(v8_context, NewV8Key(isolate, "case"), NewV8String(isolate, "promise resolve")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 0);
  }

  SECTION("promise resolve log") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "log")).IsJust();
    params->Set(v8_context, NewV8Key(isolate, "case"), NewV8String(isolate, "promise resolve")).IsJust();
    auto rst = isolate->Check(type, params, context);
    auto str = std::string(
        *v8::String::Utf8Value(isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked()));
    REQUIRE(str == R"([{"action":"log","message":"","name":"test","confidence":0}])");
  }

  SECTION("promise reject") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "log")).IsJust();
    params->Set(v8_context, NewV8Key(isolate, "case"), NewV8String(isolate, "promise reject")).IsJust();
    auto rst = isolate->Check(type, params, context);
    auto str = std::string(
        *v8::String::Utf8Value(isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked()));
    REQUIRE_THAT(str, Catch::Matchers::Matches(".*action.*exception.*"));
    // vs error C2017: illegal escape sequence
    // REQUIRE(str == R"([{"action":"exception","message":"{\"action\":\"log\"}","name":"test","confidence":0}])");
  }
}

TEST_CASE("Plugins") {
  Snapshot snapshot("global.checkPoints=['request','requestEnd'];",
                    {{"test1", R"(
        const plugin = new RASP('test1')
        plugin.register('request', (params) => {
            return params
        })
    )"},
                     {"test2", R"(
        const plugin = new RASP('test2')
        plugin.register('request', (params) => {
            return { action: 'log' }
        })
        plugin.register('requestEnd', (params) => {})
        plugin.register('xxxx', (params) => {})
    )"}},
                    "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();

  auto data = isolate->GetData();
  REQUIRE(data->check_points.size() == 2);
  REQUIRE(message == "[test2] Unknown check point name 'xxxx'\n");

  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);
  auto type = NewV8String(isolate, "request");
  auto params = v8::Object::New(isolate);
  auto context = v8::Object::New(isolate);

  SECTION("ignore") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "ignore")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 1);
  }

  SECTION("log") {
    params->Set(v8_context, NewV8Key(isolate, "action"), NewV8String(isolate, "log")).IsJust();
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 2);
  }
}

TEST_CASE("AsyncRequest") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);
  AsyncRequest::ConfigInstance(10, 1000);
  message = "";
  Platform::logger = [](const std::string& msg) {
    std::lock_guard<std::mutex> lock(mtx);
    message += msg;
  };

  SECTION("undefined config") {
    isolate->ExecScript(
        R"(
  for (let i = 0; i < 10; i++) { RASP.request_async() }
          )",
        "test");
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE_THAT(message,
                 Catch::Matchers::Contains(
                     "async request failed; url: , errMsg: Uncaught TypeError: Cannot convert undefined or null to object\n"
                     "async request failed; url: , errMsg: Uncaught TypeError: Cannot convert undefined or null to object\n"));
  }

  SECTION("400", "[!mayfail]") {
    isolate->ExecScript(
        R"(
  RASP.request_async({url: 'https://httpbin.org/status/400'})
            )",
        "test");
    for (int i = 0; i < 50; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (message.find("400") != -1) {
        break;
      }
    }
    REQUIRE_THAT(message, Catch::Matchers::Contains("async request status: 400"));
  }

  SECTION("ok", "[!mayfail]") {
    isolate->ExecScript(
        R"(
        for (let i = 0; i < 4; i++) { RASP.request_async({url: 'baidu.com'}) }
            )",
        "test");
    for (int i = 0; i < 10; i++) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      if (!message.empty()) {
        break;
      }
    }
    REQUIRE_THAT(message, Catch::Matchers::Contains(""));
  }

  SECTION("AsyncRequest") {
    AsyncRequest::GetInstance().Terminate();
    auto req = std::make_shared<HTTPRequest>();
    req->SetUrl("https://www.httpbin.org/get");
    req->SetMethod("get");
    REQUIRE(AsyncRequest::GetInstance().GetQueueSize() == 0);
    REQUIRE(AsyncRequest::GetInstance().Submit(req));
  }

  Platform::logger = plugin_log;
}

TEST_CASE("ThreadPool") {
  SECTION("basic") {
    auto pool = new ThreadPool(10, 21);
    std::atomic<int> sum(0);
    for (int i = 0; i < 20; i++) {
      pool->Post([&sum, i] { sum.fetch_add(i); });
    }
    std::promise<void> pro;
    pool->Post([&pro] { pro.set_value(); });
    pro.get_future().get();
    REQUIRE(sum.load() == 190);
  }

  SECTION("full") {
    auto pool = new ThreadPool(1, 10);
    std::promise<void> pro;
    pool->Post([&pro] {
      pro.set_value();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    });
    pro.get_future().get();
    for (int i = 0; i < 10; i++) {
      REQUIRE(pool->Post([] {}));
    }
    for (int i = 0; i < 10; i++) {
      REQUIRE(!pool->Post([] {}));
    }
  }

  SECTION("terminate") {
    auto pool = new ThreadPool(10, 100);
    for (int i = 0; i < 10; i++) {
      pool->Post([] { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });
    }
    std::promise<void> pro;
    pool->Post([&pro] { pro.set_value(); });
    for (int i = 0; i < 10; i++) {
      pool->Post([] { std::this_thread::sleep_for(std::chrono::milliseconds(10)); });
    }
    for (int i = 0; i < 10; i++) {
      pool->Post([] { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); });
    }
    pro.get_future().get();
    auto begin = std::chrono::high_resolution_clock::now();
    delete pool;
    auto end = std::chrono::high_resolution_clock::now();
    auto dur = end - begin;
    REQUIRE(dur.count() < 100 * 1000 * 1000);
  }
}

TEST_CASE("HeapLimit") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  isolate->Initialize();
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
  v8::Context::Scope context_scope(v8_context);

  auto t = new char[1024 * 1024 * 10];
  auto a = NewV8String(isolate, t, 1024 * 1024 * 10);
  delete[] t;
  v8::TryCatch try_catch(isolate);
  isolate->ExecScript(R"==(
        let arr = []
        let i = 0
        for (;;) {
          arr.push(new Array(i).fill(i))
          i++
        }
      )==",
                      "limit");
  REQUIRE(isolate->GetData()->is_oom);
  Exception e(isolate, try_catch);
  REQUIRE(e == "Terminated\n");
}

#ifndef _WIN32
#include <sys/wait.h>
int wait(pid_t pid) {
  int status;
  // Wait for the child process to exit.  This returns immediately if
  // the child has already exited. */
  while (waitpid(pid, &status, 0) < 0) {
    switch (errno) {
      case ECHILD:
        return 0;
      case EINTR:
        break;
      default:
        return -1;
    }
  }

  if (WIFEXITED(status)) {
    // The child exited normally; get its exit code.
    return WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    // The child exited because of a signal
    // The best value to return is 0x80 + signal number,
    // because that is what all Unix shells do, and because
    // it allows callers to distinguish between process exit and
    // process death by signal.
    return 0x80 + WTERMSIG(status);
  } else {
    // Unknown exit code; pass it through
    return status;
  }
}
TEST_CASE("Fork") {
  Snapshot snapshot("global.fork = true;", std::vector<PluginFile>(), "1.2.3", 1000);
  Platform::Get()->Shutdown();
  AsyncRequest::Terminate();
  pid_t pid = fork();
  REQUIRE(pid >= 0);
  if (pid != 0) {
    REQUIRE(wait(pid) == 0);
  } else {
    // freopen("/dev/null", "w", stdout);
    Platform::Get()->Startup();
    auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
    REQUIRE(isolate != nullptr);
    IsolatePtr ptr(isolate);
    isolate->Initialize();
    v8::Isolate::Scope isolate_scope(isolate);
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> v8_context = isolate->GetData()->context.Get(isolate);
    v8::Context::Scope context_scope(v8_context);
    v8::TryCatch try_catch(isolate);
    auto rst = isolate->ExecScript("global.fork", "test").ToLocalChecked();
    REQUIRE(rst->IsTrue());

    std::promise<void> pro;
    Platform::Get()->CallOnWorkerThread(std::unique_ptr<v8::Task>(new TimeoutTask(isolate, pro.get_future(), 100)));
    isolate->ExecScript("for(;;);", "loop");
    pro.set_value();
    REQUIRE(isolate->GetData()->is_timeout);
  }
}
#endif