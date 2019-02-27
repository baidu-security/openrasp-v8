#include "header.h"

using namespace openrasp;

bool ExtractBuildinAction(Isolate *isolate, std::map<std::string, std::string> &buildin_action_map)
{
    v8::HandleScope handle_scope(isolate);
    auto rst = isolate->ExecScript(R"(
        Object.keys(RASP.algorithmConfig || {})
            .filter(key => typeof key === 'string' && typeof RASP.algorithmConfig[key] === 'object' && typeof RASP.algorithmConfig[key].action === 'string')
            .map(key => [key, RASP.algorithmConfig[key].action])
    )",
                                   "extract-buildin-action");
    if (rst.IsEmpty() ||
        !rst.ToLocalChecked()->IsArray())
    {
        return false;
    }
    auto arr = rst.ToLocalChecked().As<v8::Array>();
    auto len = arr->Length();
    for (size_t i = 0; i < len; i++)
    {
        v8::HandleScope handle_scope(isolate);
        auto item = arr->Get(i).As<v8::Array>();
        v8::String::Utf8Value key(isolate, item.As<v8::Array>()->Get(0));
        v8::String::Utf8Value value(isolate, item.As<v8::Array>()->Get(1));
        auto iter = buildin_action_map.find({*key, static_cast<size_t>(key.length())});
        if (iter != buildin_action_map.end())
        {
            iter->second = std::string(*value, static_cast<size_t>(value.length()));
        }
    }
    return true;
}

bool ExtractCallableBlacklist(Isolate *isolate, std::vector<std::string> &callable_blacklist)
{
    v8::HandleScope handle_scope(isolate);
    auto context = isolate->GetCurrentContext();
    auto rst = isolate->ExecScript(R"(
        (function () {
            let blacklist
            try {
                blacklist = RASP.algorithmConfig.webshell_callable.functions
            } finally {
                if (!Array.isArray(blacklist)) {
                    blacklist = ["system", "exec", "passthru", "proc_open", "shell_exec", "popen", "pcntl_exec", "assert"]
                }
                return blacklist.filter(item => typeof item === 'string')
            }
        })()
    )",
                                   "extract-callable-blacklist");
    if (rst.IsEmpty() ||
        !rst.ToLocalChecked()->IsArray())
    {
        return false;
    }
    callable_blacklist.clear();
    auto arr = rst.ToLocalChecked().As<v8::Array>();
    auto len = arr->Length();
    for (size_t i = 0; i < len; i++)
    {
        v8::HandleScope handle_scope(isolate);
        auto item = arr->Get(i);
        v8::String::Utf8Value value(isolate, item);
        callable_blacklist.emplace_back(*value, static_cast<size_t>(value.length()));
    }
    return true;
}

bool ExtractXSSConfig(Isolate *isolate, std::string &filter_regex, int64_t &min_length, int64_t &max_detection_num)
{
    v8::HandleScope handle_scope(isolate);
    auto rst = isolate->ExecScript(R"(
        (function () {
            let filter_regex = "<![\\-\\[A-Za-z]|<([A-Za-z]{1,12})[\\/ >]"
            let min_length = 15
            let max_detection_num = 10
            try {
                let xss_userinput = RASP.algorithmConfig.xss_userinput
                if (typeof xss_userinput.filter_regex === 'string') {
                    filter_regex = xss_userinput.filter_regex
                }
                if (Number.isInteger(xss_userinput.min_length)) {
                    min_length = xss_userinput.min_length
                }
                if (Number.isInteger(xss_userinput.max_detection_num)) {
                    max_detection_num = xss_userinput.max_detection_num
                }
            } finally {
                return [filter_regex, min_length, max_detection_num]
            }
        })()
    )",
                                   "extract-xss-config");
    if (rst.IsEmpty() ||
        !rst.ToLocalChecked()->IsArray())
    {
        return false;
    }
    auto arr = rst.ToLocalChecked().As<v8::Array>();
    auto context = isolate->GetCurrentContext();
    v8::String::Utf8Value value(isolate, arr->Get(0));
    filter_regex = std::string(*value, static_cast<size_t>(value.length()));
    min_length = arr->Get(1)->IntegerValue(context).ToChecked();
    max_detection_num = arr->Get(2)->IntegerValue(context).ToChecked();
    return true;
}