#include "Pattern.hpp"
#include "QBotUtils.hpp"
#include <string>
#include <cstdlib>
#include <exception>
#include <cpp-httplib/httplib.h>
#include <rapidjson/document.h>
// #include <Windows.h>

std::string Pattern::invalid_param_reply = "";
std::string Pattern::trailling_content = "";

Pattern::Pattern(const bool is_enable, const char* prefix_keyword, const int req_type, const char* req_url_pattern) :
    is_enable(is_enable), prefix_keyword(prefix_keyword), req_type(HTTP_REQUEST_TYPE(req_type)), req_url_pattern(req_url_pattern){}

Pattern::~Pattern() {
    for (auto &reply_pattern: reply_patterns) {
        delete reply_pattern.second;
    }
}

const std::string& Pattern::get_reply_msg(const std::vector<std::string> &params) {
    rapidjson::Value root_json_obj;
    std::string result;
    int status_code, temp_num = 0;
    size_t parse_index = 0, last_index = 0, random_index;
    switch(req_type) {
        case GET:
            // make request and write data to document
            break;
        case POST:
            throw(std::exception("POST 方法尚未实现，不允许使用"));
            break;
        case AURO_REPLY:
            // do nothing about document
            // get the first availble status_code as chosen
            if (!reply_patterns.empty()) status_code = reply_patterns.begin()->first;
            break;
        default:
            throw(std::exception("不合法的回复解析模式，请检查数据文件或设置是否错误"));
    }
    if (reply_patterns.empty()) return std::string("");
    auto &reply_pattern_iter = reply_patterns.find(status_code);
    if (reply_pattern_iter == reply_patterns.end() || reply_pattern_iter->second->size() == 0) throw("没有可用的回复模板");
    
    // 随机选择回复模板
    random_index = std::rand() % reply_pattern_iter->second->size();
    const std::string &reply_pattern = reply_pattern_iter->second->at(random_index);

    // 根据给定参数解析并填充模板
    for (parse_index = 0; parse_index < reply_pattern.size(); ++parse_index) {
        switch(reply_pattern[parse_index]) {
            case '[':
                if (++parse_index < reply_pattern.size() && reply_pattern[parse_index] == '$') {
                    result += reply_pattern.substr(last_index, parse_index++ - last_index - 1);
                    if (parse_index > reply_pattern.size()) {
                        last_index = parse_index - 2;
                        break;
                    }
                    temp_num = reply_pattern[parse_index] - '0';
                    if (reply_pattern[++parse_index] != ']') {
                        last_index = parse_index - 2;
                        break;
                    }
                    if (temp_num < 0 || temp_num > 9) throw(std::exception("解析错误：不合法的回复模板"));
                    // ++temp_num cuz the prefix keywords will be count as params[0]
                    if (++temp_num > params.size()) return invalid_param_reply;
                    result += params[temp_num];
                    last_index = parse_index + 1; 
                } else --parse_index;
            break;
            case '{':
                if (++parse_index < reply_pattern.size() && reply_pattern[parse_index] == '$') {
                    result += reply_pattern.substr(last_index, parse_index - last_index - 1);
                    last_index = ++parse_index;
                    while (parse_index < reply_pattern.size() && reply_pattern[parse_index] != ']') ++parse_index;
                    if (parse_index >= reply_pattern.size()) throw(std::exception("解析错误：不合法的回复模板"));
                    const rapidjson::Value &json_field = QBot_Utils::get_nested_json(root_json_obj, reply_pattern.substr(last_index, parse_index - last_index));
                    if (json_field.IsString()) {
                        result += json_field.GetString();
                    } else if (json_field.IsInt()) {
                        result += std::to_string(json_field.GetInt());
                    } else if (json_field.IsDouble()) {
                        result += std::to_string(json_field.GetDouble());
                    } else throw(std::exception("解析错误：目标JSON不为可以直接输出的类型（String、Number）"));
                    last_index = parse_index + 1;
                } else --parse_index;
            break;
            case '\\':
                result += reply_pattern.substr(last_index, parse_index++ - last_index);
                last_index = parse_index;
            default:
            break;
        }
    }
    if (last_index < reply_pattern.size()) result += reply_pattern.substr(last_index);
    result += trailling_content;
    return result;
}