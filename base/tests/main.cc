#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do
                           // this in one cpp file
#include "catch2/catch.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include "../bundle.h"
using namespace openrasp;

void openrasp::plugin_info(Isolate* isolate, const std::string& message) {}
void openrasp::alarm_info(Isolate* isolate,
                          v8::Local<v8::String> type,
                          v8::Local<v8::Object> params,
                          v8::Local<v8::Object> result) {}
v8::Local<v8::ObjectTemplate> openrasp::CreateRequestContextTemplate(Isolate* isolate) {
  return v8::ObjectTemplate::New(isolate);
}

class IsolateDeleter {
 public:
  void operator()(openrasp::Isolate* isolate) { isolate->Dispose(); }
};
typedef std::unique_ptr<openrasp::Isolate, IsolateDeleter> IsolatePtr;

struct Listener : Catch::TestEventListenerBase {
  using TestEventListenerBase::TestEventListenerBase;  // inherit constructor
  void testRunStarting(Catch::TestRunInfo const& testRunInfo) override {
    v8::V8::InitializePlatform(Platform::New(0));
    v8::V8::Initialize();
  }
  void testRunEnded(Catch::TestRunStats const& testRunStats) override {
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
  }
};
CATCH_REGISTER_LISTENER(Listener);

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

  Snapshot snapshot("", std::vector<PluginFile>(), 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  isolate->Dispose();
}

TEST_CASE("Snapshot") {
  SECTION("Constructor 1") {
    {
      Snapshot snapshot("", std::vector<PluginFile>(), 1000, nullptr);
      REQUIRE(snapshot.data != nullptr);
      REQUIRE(snapshot.raw_size > 0);
      REQUIRE(snapshot.timestamp == 1000);
    }
    {
      Snapshot snapshot("wrong syntax", std::vector<PluginFile>(), 1000, nullptr);
      REQUIRE(snapshot.data != nullptr);
      REQUIRE(snapshot.raw_size > 0);
      REQUIRE(snapshot.timestamp == 1000);
    }
    {
      Snapshot snapshot("", {{"wrong-syntax", "wrong syntax"}}, 1000, nullptr);
      REQUIRE(snapshot.data != nullptr);
      REQUIRE(snapshot.raw_size > 0);
      REQUIRE(snapshot.timestamp == 1000);
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
    Snapshot snapshot1("", std::vector<PluginFile>(), 1000, nullptr);
    snapshot1.Save("/tmp/openrasp-v8-base-tests-snapshot");
    Snapshot snapshot2("/tmp/openrasp-v8-base-tests-snapshot", 1000);
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
    REQUIRE_FALSE(snapshot.Save("/not-exist/not-exist/not-exist"));
  }
}

TEST_CASE("Isolate") {
  Snapshot snapshot("", {{"test", R"(
        const plugin = new RASP('test')
        plugin.register('request', (params) => {
            if (params.throw) { a.a() }
            if (params.timeout) { for(;;) {} }
            return params
        })
    )"}},
                    1000);
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
    auto context = isolate->GetData()->request_context_templ.Get(isolate)->NewInstance();

    {
      params->Set(NewV8String(isolate, "action"), NewV8String(isolate, "ignore"));
      auto rst = isolate->Check(type, params, context);
      REQUIRE(rst->Length() == 0);
    }

    {
      params->Set(NewV8String(isolate, "action"), NewV8String(isolate, "log"));
      auto rst = isolate->Check(type, params, context);
      REQUIRE(std::string(*v8::String::Utf8Value(
                  isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
              R"([{"action":"log","message":"","name":"test","confidence":0}])");
    }

    {
      params->Set(NewV8String(isolate, "action"), NewV8String(isolate, "block"));
      auto rst = isolate->Check(type, params, context);
      REQUIRE(std::string(*v8::String::Utf8Value(
                  isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
              R"([{"action":"block","message":"","name":"test","confidence":0}])");
    }

    {
      params->Set(NewV8String(isolate, "timeout"), v8::Boolean::New(isolate, true));
      auto rst = isolate->Check(type, params, context);
      REQUIRE(std::string(*v8::String::Utf8Value(
                  isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked())) ==
              R"([{"action":"exception","message":"Javascript plugin execution timeout"}])");
    }

    {
      params->Set(NewV8String(isolate, "throw"), v8::Boolean::New(isolate, true));
      auto rst = isolate->Check(type, params, context);
      auto str = std::string(
          *v8::String::Utf8Value(isolate, v8::JSON::Stringify(isolate->GetCurrentContext(), rst).ToLocalChecked()));
      REQUIRE_THAT(str, Catch::Matchers::Matches(".*exception.*a is not defined.*"));
    }
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
  Snapshot snapshot("", std::vector<PluginFile>(), 1000);
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
  Snapshot snapshot("", std::vector<PluginFile>(), 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  IsolatePtr ptr(isolate);
  v8::HandleScope handle_scope(isolate);

  SECTION("Not Timeout") {
    auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    auto task = new TimeoutTask(isolate, 1000);
    task->GetMtx().lock();
    Platform::Get()->CallOnWorkerThread(std::unique_ptr<v8::Task>(task));
    isolate->ExecScript("for(let i=0;i<10;i++);", "loop");
    task->GetMtx().unlock();
    auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    REQUIRE(end - start <= 1000);
  }

  SECTION("Timeout") {
    auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    auto task = new TimeoutTask(isolate, 100);
    task->GetMtx().lock();
    Platform::Get()->CallOnWorkerThread(std::unique_ptr<v8::Task>(task));
    isolate->ExecScript("for(;;);", "loop");
    task->GetMtx().unlock();
    auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    REQUIRE(end - start >= 100);
  }
}

TEST_CASE("Flex") {
  Snapshot snapshot("", std::vector<PluginFile>(), 1000);
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