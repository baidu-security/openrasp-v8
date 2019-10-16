#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include "header.h"

class IsolateDeleter {
 public:
  ALIGN_FUNCTION void operator()(openrasp_v8::Isolate* isolate) { isolate->Dispose(); }
};

std::shared_ptr<openrasp_v8::Isolate> GetIsolate() {
  static std::mutex mtx;
  static std::list<std::weak_ptr<openrasp_v8::Isolate>> isolates;

  std::lock_guard<std::mutex> lock1(mtx);
  isolates.remove_if([](std::weak_ptr<openrasp_v8::Isolate> ptr) { return ptr.use_count() == 0; });
  for (auto ptr : isolates) {
    if (ptr.use_count() < 5) {
      auto isolate = ptr.lock();
      if (!isolate->IsExpired(snapshot->timestamp)) {
        return isolate;
      }
    }
  }

  std::lock_guard<std::mutex> lock2(snapshot_mtx);
  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  auto isolate = openrasp_v8::Isolate::New(snapshot, millis);
  v8::Locker lock3(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  isolate->Initialize();
  isolate->GetData()->request_context_templ.Reset(isolate, CreateRequestContextTemplate(isolate));

  std::shared_ptr<openrasp_v8::Isolate> ptr(isolate, IsolateDeleter());
  isolates.push_front(ptr);

  return ptr;
}