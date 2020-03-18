#include "bundle.h"
#include "flex/flex.h"
#include "request.h"

namespace openrasp_v8 {

void log_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  Isolate* isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  for (int i = 0; i < info.Length(); i++) {
    v8::String::Utf8Value message(isolate, info[i]);
    Platform::logger({*message, static_cast<size_t>(message.length())});
  }
}

void flex_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  Isolate* isolate = reinterpret_cast<Isolate*>(info.GetIsolate());
  auto context = isolate->GetCurrentContext();
  if (info.Length() < 2 || !info[0]->IsString() || !info[1]->IsString()) {
    return;
  }
  v8::String::Utf8Value str(isolate, info[0]);
  v8::String::Utf8Value lexer_mode(isolate, info[1]);

  char* input = *str;
  int input_len = str.length();

  flex_token_result token_result = flex_lexing(input, input_len, *lexer_mode);

  size_t len = std::min(uint32_t(input_len), token_result.result_len);
  auto arr = v8::Array::New(isolate, len);
  for (int i = 0; i < len; i++) {
    arr->Set(context, i, v8::Integer::New(isolate, token_result.result[i])).IsJust();
  }
  free(token_result.result);
  info.GetReturnValue().Set(arr);
}

void request_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  v8::TryCatch try_catch(isolate);
  auto context = isolate->GetCurrentContext();
  v8::Local<v8::Promise::Resolver> resolver;
  if (!v8::Promise::Resolver::New(context).ToLocal(&resolver)) {
    try_catch.ReThrow();
    return;
  }
  info.GetReturnValue().Set(resolver->GetPromise());
  HTTPRequest req(isolate, info[0]);
  HTTPResponse res = req.GetResponse();
  auto object = res.ToObject(isolate);
  if (res.error) {
    resolver->Reject(context, object).IsJust();
  } else {
    resolver->Resolve(context, object).IsJust();
  }
}

void request_async_callback(const v8::FunctionCallbackInfo<v8::Value>& info) {
  auto isolate = info.GetIsolate();
  AsyncRequest::GetInstance().Submit(std::make_shared<HTTPRequest>(isolate, info[0]));
}

intptr_t* Snapshot::external_references = new intptr_t[5]{
    reinterpret_cast<intptr_t>(log_callback),
    reinterpret_cast<intptr_t>(flex_callback),
    reinterpret_cast<intptr_t>(request_callback),
    reinterpret_cast<intptr_t>(request_async_callback),
    0,
};
}  // namespace openrasp_v8