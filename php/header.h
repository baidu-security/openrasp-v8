#include "base/bundle.h"

bool ExtractBuildinAction(openrasp::Isolate *isolate, std::map<std::string, std::string> &buildin_action_map);
bool ExtractCallableBlacklist(openrasp::Isolate *isolate, std::vector<std::string> &callable_blacklist);
bool ExtractXSSConfig(openrasp::Isolate *isolate, std::string &filter_regex, int64_t &min_length, int64_t &max_detection_num);