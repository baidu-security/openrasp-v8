#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include "catch2/catch.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include "../bundle.h"

using namespace openrasp;

void openrasp::plugin_info(Isolate* isolate, const std::string& message) {
  // std::cout << message << std::endl;
}

class IsolateDeleter {
 public:
  void operator()(openrasp::Isolate* isolate) { isolate->Dispose(); }
};
typedef std::unique_ptr<openrasp::Isolate, IsolateDeleter> IsolatePtr;

struct Listener : Catch::TestEventListenerBase {
  using TestEventListenerBase::TestEventListenerBase;  // inherit constructor
  void testRunStarting(Catch::TestRunInfo const& testRunInfo) override { Initialize(0); }
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

  BENCHMARK("ignore", i) {
    v8::HandleScope handle_scope(isolate);
    auto type = NewV8String(isolate, "request");
    auto json = NewV8String(isolate,
                            R"({"action":"ignore","message":"1234567890","name":"test","confidence":0})");
    auto params = v8::JSON::Parse(isolate->GetCurrentContext(), json).ToLocalChecked().As<v8::Object>();
    auto context = v8::Object::New(isolate);
    context->Set(NewV8Key(isolate, "index"), v8::Int32::New(isolate, i));
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
    v8::HandleScope handle_scope(isolate);
    auto type = NewV8String(isolate, "request");
    auto json = NewV8String(isolate,
                            R"({"action":"log","message":"1234567890","name":"test","confidence":0})");
    auto params = v8::JSON::Parse(isolate->GetCurrentContext(), json).ToLocalChecked().As<v8::Object>();
    auto context = v8::Object::New(isolate);
    context->Set(NewV8Key(isolate, "index"), v8::Int32::New(isolate, i));
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
      v8::HandleScope handle_scope(isolate);
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
  v8::HandleScope handle_scope(isolate);

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
  v8::HandleScope handle_scope(isolate);

  v8::TryCatch try_catch(isolate);
  isolate->ExecScript("throw new Error('2333')", "2333.js");
  REQUIRE(try_catch.HasCaught());
  Exception e(isolate, try_catch);
  REQUIRE(e == R"(2333.js:1
throw new Error('2333')
^
Error: 2333
    at 2333.js:1:7
)");
}

TEST_CASE("TimeoutTask") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  v8::HandleScope handle_scope(isolate);

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
  v8::HandleScope handle_scope(isolate);
  auto maybe_rst = isolate->ExecScript("JSON.stringify(RASP.sql_tokenize('a bb       ccc dddd   '))", "flex");
  REQUIRE_FALSE(maybe_rst.IsEmpty());
  REQUIRE(maybe_rst.ToLocalChecked()->IsString());
  REQUIRE(
      std::string(*v8::String::Utf8Value(isolate, maybe_rst.ToLocalChecked())) ==
      R"([{"start":0,"stop":1,"text":"a"},{"start":2,"stop":4,"text":"bb"},{"start":11,"stop":14,"text":"ccc"},{"start":15,"stop":19,"text":"dddd"}])");
}

TEST_CASE("Request", "[!mayfail]") {
  Snapshot snapshot("", std::vector<PluginFile>(), "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  IsolatePtr ptr(isolate);
  v8::HandleScope handle_scope(isolate);
  auto context = isolate->GetCurrentContext();
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
  }
  SECTION("timeout") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request({
    url: 'https://www.httpbin.org/get',
    timeout: 10
}).catch(err => Promise.reject(JSON.stringify(err)))
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kRejected);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("code":8)==="));
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
    REQUIRE_THAT(rst, Catch::Matchers::Contains(R"===("Location":"/relative-redirect/1")==="));
  }
  SECTION("undefined config") {
    auto maybe_rst = isolate->ExecScript(
        R"(
RASP.request()
          )",
        "request");
    auto promise = maybe_rst.ToLocalChecked().As<v8::Promise>();
    REQUIRE(promise->State() == v8::Promise::PromiseState::kRejected);
    auto rst = std::string(*v8::String::Utf8Value(isolate, promise->Result()));
    REQUIRE(rst == "TypeError: Cannot convert undefined or null to object");
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
  v8::HandleScope handle_scope(isolate);
  auto type = NewV8String(isolate, "request");
  auto params = v8::Object::New(isolate);
  auto context = v8::Object::New(isolate);

  SECTION("ignore") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "ignore"));
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 0);
  }

  SECTION("log") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "log"));
    auto rst = isolate->Check(type, params, context);
    REQUIRE(std::string(*v8::String::Utf8Value(
                isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
            R"([{"action":"log","message":"","name":"test","confidence":0}])");
  }

  SECTION("block") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "block"));
    auto rst = isolate->Check(type, params, context);
    REQUIRE(std::string(*v8::String::Utf8Value(
                isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
            R"([{"action":"block","message":"","name":"test","confidence":0}])");
  }

  SECTION("timeout") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "log"));
    params->Set(NewV8Key(isolate, "case"), NewV8String(isolate, "timeout"));
    auto rst = isolate->Check(type, params, context);
    REQUIRE(std::string(*v8::String::Utf8Value(
                isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
            R"([{"action":"exception","message":"Javascript plugin execution timeout"}])");
  }

  SECTION("throw") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "log"));
    params->Set(NewV8Key(isolate, "case"), NewV8String(isolate, "throw"));
    auto rst = isolate->Check(type, params, context);
    auto str = std::string(
        *v8::String::Utf8Value(isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked()));
    REQUIRE_THAT(str, Catch::Matchers::Matches(".*exception.*a is not defined.*"));
  }

  SECTION("promise resolve ignore") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "ignore"));
    params->Set(NewV8Key(isolate, "case"), NewV8String(isolate, "promise resolve"));
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 0);
  }

  SECTION("promise resolve log") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "log"));
    params->Set(NewV8Key(isolate, "case"), NewV8String(isolate, "promise resolve"));
    auto rst = isolate->Check(type, params, context);
    auto str = std::string(
        *v8::String::Utf8Value(isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked()));
    REQUIRE(str == R"([{"action":"log","message":"","name":"test","confidence":0}])");
  }

  SECTION("promise reject") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "log"));
    params->Set(NewV8Key(isolate, "case"), NewV8String(isolate, "promise reject"));
    auto rst = isolate->Check(type, params, context);
    auto str = std::string(
        *v8::String::Utf8Value(isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked()));
    REQUIRE_THAT(str, Catch::Matchers::Matches(".*action.*exception.*"));
    // vs error C2017: illegal escape sequence
    // REQUIRE(str == R"([{"action":"exception","message":"{\"action\":\"log\"}","name":"test","confidence":0}])");
  }
}

TEST_CASE("Plugins") {
  Snapshot snapshot("",
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
    )"}},
                    "1.2.3", 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  v8::HandleScope handle_scope(isolate);
  auto type = NewV8String(isolate, "request");
  auto params = v8::Object::New(isolate);
  auto context = v8::Object::New(isolate);

  SECTION("ignore") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "ignore"));
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 1);
  }

  SECTION("log") {
    params->Set(NewV8Key(isolate, "action"), NewV8String(isolate, "log"));
    auto rst = isolate->Check(type, params, context);
    REQUIRE(rst->Length() == 2);
  }
}