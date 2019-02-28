#include "../bundle.h"
using namespace openrasp;

void openrasp::plugin_info(Isolate *isolate, const std::string &message)
{

}
void openrasp::alarm_info(Isolate *isolate, v8::Local<v8::String> type, v8::Local<v8::Object> params, v8::Local<v8::Object> result)
{

}
v8::Local<v8::ObjectTemplate> openrasp::CreateRequestContextTemplate(Isolate *isolate)
{
    return v8::ObjectTemplate::New(isolate);
}