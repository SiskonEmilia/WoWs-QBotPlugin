#include "Pattern.hpp"

Pattern::Pattern(const bool is_enable, const char* prefix_keyword, const int req_type, const char* req_url_pattern) :
    is_enable(is_enable), prefix_keyword(prefix_keyword), req_type(HTTP_REQUEST_TYPE(req_type)), req_url_pattern(req_url_pattern){}

Pattern::~Pattern() {
    for (auto &reply_pattern: reply_patterns) {
        delete reply_pattern.second;
    }
}