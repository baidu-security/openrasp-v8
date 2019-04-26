#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do
                           // this in one cpp file
#include "catch2/catch.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include "../bundle.h"
using namespace openrasp;

TEST_CASE("Platform") {
  Platform::Initialize();
  Platform::Shutdown();

  Platform::Shutdown();
  Platform::Initialize();

  Platform::Initialize();
  Platform::Shutdown();
}

TEST_CASE("Snapshot") {
  Platform::Initialize();
  v8::V8::Initialize();

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
  Platform::Initialize();
  v8::V8::Initialize();
  Snapshot snapshot("", {{"test", R"(
        const plugin = new RASP('test')
        plugin.register('request', (params) => {
            if (params.timeout) { for(;;) {} }
            return params
        })
    )"}},
                    1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);

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

  SECTION("Check1") {
    v8::HandleScope handle_scope(isolate);
    auto type = NewV8String(isolate, "request");
    auto params = v8::Object::New(isolate);

    params->Set(NewV8String(isolate, "action"), NewV8String(isolate, "ignore"));
    REQUIRE_FALSE(isolate->Check(type, params));

    params->Set(NewV8String(isolate, "action"), NewV8String(isolate, "log"));
    REQUIRE_FALSE(isolate->Check(type, params));

    params->Set(NewV8String(isolate, "action"), NewV8String(isolate, "block"));
    REQUIRE(isolate->Check(type, params));

    params->Set(NewV8String(isolate, "timeout"), v8::Boolean::New(isolate, true));
    REQUIRE_FALSE(isolate->Check(type, params));
  }

  SECTION("Check2") {
    v8::HandleScope handle_scope(isolate);
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
              R"([{"action":"log","message":"Javascript plugin execution timeout"}])");
    }
  }

  SECTION("ExecScript") {
    v8::HandleScope handle_scope(isolate);
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

  isolate->Dispose();
}

TEST_CASE("Exception") {
  Platform::Initialize();
  v8::V8::Initialize();
  Snapshot snapshot("", std::vector<PluginFile>(), 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);

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
  isolate->Dispose();
}

TEST_CASE("TimeoutTask") {
  Platform::Initialize();
  v8::V8::Initialize();
  Snapshot snapshot("", std::vector<PluginFile>(), 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);

  SECTION("Not Timeout") {
    TimeoutTask task(isolate, 100);
    auto start = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    task.Run();
    auto end = std::chrono::high_resolution_clock::now().time_since_epoch().count() / 1000;
    REQUIRE(end - start < 100);
  }

  SECTION("Timeout") {
    TimeoutTask task(isolate, 10);
    std::thread([&task] {
      std::lock_guard<std::timed_mutex> lock(task.GetMtx());
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    })
        .detach();
    std::this_thread::sleep_for(std::chrono::microseconds(10));
    auto start = std::chrono::steady_clock::now().time_since_epoch().count() / 1000;
    task.Run();
    auto end = std::chrono::steady_clock::now().time_since_epoch().count() / 1000;
    REQUIRE(end - start > 100);
  }

  isolate->Dispose();
}

TEST_CASE("Flex") {
  Platform::Initialize();
  v8::V8::Initialize();
  Snapshot snapshot("", std::vector<PluginFile>(), 1000);
  auto isolate = Isolate::New(&snapshot, snapshot.timestamp);
  REQUIRE(isolate != nullptr);
  auto maybe_rst = isolate->ExecScript("JSON.stringify(RASP.sql_tokenize('a bb       ccc dddd   '))", "flex");
  REQUIRE_FALSE(maybe_rst.IsEmpty());
  REQUIRE(maybe_rst.ToLocalChecked()->IsString());
  REQUIRE(
      std::string(*v8::String::Utf8Value(isolate, maybe_rst.ToLocalChecked())) ==
      R"([{"start":0,"stop":1,"text":"a"},{"start":2,"stop":4,"text":"bb"},{"start":11,"stop":14,"text":"ccc"},{"start":15,"stop":19,"text":"dddd"}])");
  isolate->Dispose();
}