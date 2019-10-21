#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include "header.h"

namespace isolate_pool {

class IsolateDeleter {
 public:
  ALIGN_FUNCTION void operator()(openrasp_v8::Isolate* isolate) { isolate->Dispose(); }
};

size_t size = 1;
static std::list<std::weak_ptr<openrasp_v8::Isolate>> isolates;

std::shared_ptr<openrasp_v8::Isolate> GetIsolate() {
  std::lock_guard<std::mutex> lock1(snapshot_mtx);
  isolates.remove_if([](std::weak_ptr<openrasp_v8::Isolate> ptr) {
    auto p = ptr.lock();
    return !p || p->IsDead() || p->IsExpired(snapshot->timestamp);
  });
  isolates.sort([](std::weak_ptr<openrasp_v8::Isolate> p1, std::weak_ptr<openrasp_v8::Isolate> p2) {
    return p1.use_count() < p2.use_count();
  });

  if (isolates.size() == size) {
    auto p = isolates.front().lock();
    if (p) {
      return p;
    }
  }

  auto duration = std::chrono::system_clock::now().time_since_epoch();
  auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  auto isolate = openrasp_v8::Isolate::New(snapshot, millis);
  v8::Locker lock2(isolate);
  v8::Isolate::Scope isolate_scope(isolate);
  v8::HandleScope handle_scope(isolate);
  isolate->Initialize();
  isolate->GetData()->request_context_templ.Reset(isolate, CreateRequestContextTemplate(isolate));

  std::shared_ptr<openrasp_v8::Isolate> ptr(isolate, IsolateDeleter());
  isolates.push_front(ptr);

  return ptr;
}
}  // namespace isolate_pool