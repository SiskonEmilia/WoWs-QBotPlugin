#include "Pattern.hpp"
#include "QBotUtils.hpp"
#include <string>
#include <cstdlib>
#include <exception>
#include <rapidjson/document.h>
#include <memory>
#include <regex>

#include <cqcppsdk/cqcppsdk.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cpp-httplib/httplib.h>
#include <Windows.h>

std::string Pattern::invalid_param_reply = "";
std::string Pattern::trailling_content = "";
std::string Pattern::bot_mentioned_key = "";
char Pattern::param_splitor = ' ';

std::regex url_reg(QBOT_VALID_HTTP_URL);

Pattern::Pattern(const bool is_enable, const char* prefix_keyword, const int req_type) :
    is_enable(is_enable), prefix_keyword(prefix_keyword), req_type(HTTP_REQUEST_TYPE(req_type)),
    req_url_patterns(std::vector<std::string>(10)), req_url_hosts(std::vector<std::string>(10)),
    req_url_domains(std::vector<std::string>(10)), req_url_path_patterns(std::vector<std::string>(10)) {}

Pattern::~Pattern() {
    for (auto &reply_pattern: reply_patterns) {
        delete reply_pattern.second;
    }
}

bool Pattern::verify_pattern(const std::string &reply_pattern, const int& param_count) {
    int temp_num;
    size_t parse_index = 0;
    for (parse_index = 0; parse_index < reply_pattern.size(); ++parse_index) {
        switch(reply_pattern[parse_index]) {
            case '[':
                if (++parse_index < reply_pattern.size() && reply_pattern[parse_index] == '$') {
                    temp_num = reply_pattern[parse_index] - '0';
                    if (reply_pattern[++parse_index] != ']') {
                        --parse_index;
                        break;
                    }
                    if (temp_num < 0 || ++temp_num >= param_count) return false;
                } else --parse_index;
            break;
            case '\\':
                ++parse_index;
            break;
            default:
            break;
        }
    }
    return true;
}

void fill_pattern(const std::string &reply_pattern, const std::vector<std::string> &params,
    const rapidjson::Value &root_json_obj, std::string &result, bool is_path = false) {
    int temp_num;
    size_t parse_index = 0, last_index = 0;
    bool skip_json = root_json_obj.IsNull();
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
                        last_index = parse_index-- - 2;
                        break;
                    }
                    if (temp_num < 0 || temp_num > 9) throw(std::exception("解析错误：不合法的回复模板"));
                    // ++temp_num cuz the prefix keywords will be count as params[0]
                    if (++temp_num >= params.size()) {
                        if (is_path) throw 0;
                        result = Pattern::invalid_param_reply;
                        return;
                    }
                    result += params[temp_num];
                    last_index = parse_index + 1; 
                } else --parse_index;
            break;
            case '{':
                if (skip_json) break;
                if (++parse_index < reply_pattern.size() && reply_pattern[parse_index] == '$') {
                    result += reply_pattern.substr(last_index, parse_index - last_index - 1);
                    last_index = ++parse_index;
                    while (parse_index < reply_pattern.size() && reply_pattern[parse_index] != '}') ++parse_index;
                    if (parse_index >= reply_pattern.size()) {
                        parse_index -= 2;
                        break;
                    }
                    const rapidjson::Value &json_field = QBot_Utils::get_nested_json(root_json_obj, reply_pattern.substr(last_index, parse_index - last_index));
                    if (json_field.IsString()) {
                        result += json_field.GetString();
                    } else if (json_field.IsInt()) {
                        result += std::to_string(json_field.GetInt());
                    } else if (json_field.IsDouble()) {
                        result += std::to_string(json_field.GetDouble());
                    } else throw(std::exception("解析错误：目标JSON域不为可以直接输出的类型（String、Number）"));
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
}

void Pattern::get_reply_msg(const std::vector<std::string> &params, std::string &reply) {
    int pattern_index = params.size() - 1;
    if (pattern_index > 10) {
        reply = Pattern::invalid_param_reply;
        return;
    }
    
    rapidjson::Document root_json_obj;
    int status_code;
    size_t random_index;
    switch(req_type) {
        case GET: {
            // make request and write data to document
            // do we have some cache about this?
            std::string &target_domain = req_url_domains[pattern_index],
                &target_host = req_url_hosts[pattern_index],
                &target_path = req_url_path_patterns[pattern_index];
            if (req_url_domains[pattern_index].empty()) {
                // seems not
                std::cmatch m;
                if (std::regex_match(req_url_patterns[pattern_index].c_str(), m, url_reg)) {
                    target_domain = m[1].str();
                    target_host   = m[3].str();
                    target_path   = m[5].str();
                    if (target_path.empty()) target_path = "/";
                } else throw std::exception("无法解析的 HTTP 链接，请注意需要准确填写协议头。");
                #ifdef DEBUG_VERSION
                cq::logging::debug("current_url_pattern", req_url_patterns[pattern_index]);
                #endif
            }
            httplib::Client2 cli(target_domain.c_str());
            httplib::Headers headers({
                {"Host", target_host.c_str()},
                {"Accept", "application/json;charset=utf-8"}
            });
            std::string req_path;
            try {
                fill_pattern(target_path, params, root_json_obj, req_path, true);
            } catch(const int &err) {
                reply = Pattern::invalid_param_reply;
                return;
            }
            const auto &res = cli.Get(req_path.c_str(), headers);
            if (res) {
                status_code = res->status;
                if (root_json_obj.Parse(res->body.c_str()).HasParseError()) {
                    cq::logging::warning("非JSON请求内容", res->body);
                    throw std::exception("请求返回内容不是合法的JSON数据结构。");
                }
            } else throw std::exception("网络请求失败");}
            break;
        case POST:
            throw std::exception("POST 方法尚未实现，不允许使用");
            break;
        case AURO_REPLY:
            // do nothing about document
            // get the first availble status_code as chosen
            if (!reply_patterns.empty()) status_code = reply_patterns.begin()->first;
            break;
        default:
            throw std::exception("不合法的回复解析模式，请检查数据文件或设置是否错误");
    }
    if (reply_patterns.empty()) throw("没有可用的回复模板"); // return std::string("");
    auto reply_pattern_iter = reply_patterns.find(status_code);
    if (reply_pattern_iter == reply_patterns.end() || reply_pattern_iter->second->size() == 0) throw std::exception("没有可用的回复模板");
    
    // 随机选择回复模板
    random_index = std::rand() % reply_pattern_iter->second->size();
    const std::string &reply_pattern = reply_pattern_iter->second->at(random_index);
    
    fill_pattern(reply_pattern, params, root_json_obj, reply);
    reply += Pattern::trailling_content;
}