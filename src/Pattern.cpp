#include "Pattern.hpp"
#include "QBotUtils.hpp"
#include "Renderer.hpp"

#include <string>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>

#include <rapidjson/document.h>
#include <cqcppsdk/cqcppsdk.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <cpp-httplib/httplib.h>
#include <Windows.h>

std::string Pattern::invalid_param_reply = "";
std::string Pattern::trailling_content = "";
std::string Pattern::bot_mentioned_key = "";
std::string Pattern::i_need_help = "";
std::string Pattern::network_issue_report = "";
char Pattern::param_splitor = ' ';

// #define TEST_CLIENT_POOL

#ifdef TEST_CLIENT_POOL
#define CP_DEFAULT_DOMAIN "https://www.mgaia.top"
// We put definition here cuz "httplib.h" includes "openssl/applink.c"
// which will introduce multiple-definition error
// As any one of the httplib::Client2 instance is not thread-safe
// we design this class to make sure every single instance is occupied
// by only one thread
class TSClientPool {
private:
    std::vector<httplib::Client2> clis;
    std::vector<std::string> cli_domains;
    std::vector<int> cli_indexs;
    std::mutex mutex_lock;
    int cli_stack_top;
public:
    TSClientPool();
    void get_cli_index(int& cli_index);
    void put_cli_back(const int& cli_index);
    httplib::Client2& get_cli_instance(const int& cli_index, const std::string& cli_domain);
} client_pool;

TSClientPool::TSClientPool() : clis(MAX_AVALIBEL_THREAD, httplib::Client2(CP_DEFAULT_DOMAIN)),
    cli_stack_top(MAX_AVALIBEL_THREAD - 1), cli_indexs(MAX_AVALIBEL_THREAD, 0),
    cli_domains(MAX_AVALIBEL_THREAD, CP_DEFAULT_DOMAIN) {
    for (size_t i = 0; i < MAX_AVALIBEL_THREAD; ++i) cli_indexs[i] = i;
}

void TSClientPool::get_cli_index(int& cli_index) {
    mutex_lock.lock();
    #ifdef DEBUG_VERSION
    cq::logging::debug("GET, top", std::to_string(cli_stack_top));
    #endif
    if (cli_stack_top < 0) cli_index = -1;
    else cli_index = cli_indexs[cli_stack_top--];
    mutex_lock.unlock();
};

void TSClientPool::put_cli_back(const int& cli_index) {
    mutex_lock.lock();
    #ifdef DEBUG_VERSION
    cq::logging::debug("PUT, top", std::to_string(cli_stack_top));
    #endif
    if (cli_index < MAX_AVALIBEL_THREAD)
        cli_indexs[++cli_stack_top] = cli_index;
    else
        throw std::exception("Out of range cli_index");
    mutex_lock.unlock();
}

httplib::Client2& TSClientPool::get_cli_instance(const int& cli_index, const std::string& cli_domain) {
    if (cli_index < MAX_AVALIBEL_THREAD){
        if (cli_domains[cli_index] != cli_domain) {
            cli_domains[cli_index] = cli_domain;
            clis[cli_index] = httplib::Client2(cli_domain.c_str());
        }
        return clis[cli_index];
    }
    else throw std::exception("Out of range cli_index");
};

#endif

std::regex Pattern::url_reg(QBOT_VALID_HTTP_URL);

Pattern::Pattern(){}

Pattern::Pattern(const bool is_enable, const char* prefix_keyword, const int req_type) :
    min_param_count(10),
    is_enable(is_enable), prefix_keyword(prefix_keyword), req_type(HTTP_REQUEST_TYPE(req_type)),
    req_url_patterns(std::vector<std::string>((req_type == AUTO_REPLY) ? 0 : 11)), 
    req_url_hosts(std::vector<std::string>((req_type == AUTO_REPLY) ? 0 : 11)),
    req_url_domains(std::vector<std::string>((req_type == AUTO_REPLY) ? 0 : 11)), 
    req_path_renderers(std::vector<RendererObject>((req_type == AUTO_REPLY) ? 0 : 11)) {}

Pattern::~Pattern() {}

// DEPRECATED FUNCTION
// bool Pattern::verify_pattern(const std::string &reply_pattern, const int& param_count) {
//     int temp_num;
//     size_t parse_index = 0;
//     for (parse_index = 0; parse_index < reply_pattern.size(); ++parse_index) {
//         switch(reply_pattern[parse_index]) {
//             case '[':
//                 if (++parse_index < reply_pattern.size() && reply_pattern[parse_index] == '$') {
//                     temp_num = reply_pattern[parse_index] - '0';
//                     if (reply_pattern[++parse_index] != ']') {
//                         --parse_index;
//                         break;
//                     }
//                     if (temp_num < 0 || ++temp_num >= param_count) return false;
//                 } else --parse_index;
//             break;
//             case '\\':
//                 ++parse_index;
//             break;
//             default:
//             break;
//         }
//     }
//     return true;
// }

// DEPRECATED FUNCTION
// void fill_pattern(const std::string &reply_pattern, const std::vector<std::string> &params,
//     const rapidjson::Value &root_json_obj, std::string &result, bool is_path = false) {
//     int temp_num;
//     size_t parse_index = 0, last_index = 0;
//     bool skip_json = root_json_obj.IsNull();
//     for (parse_index = 0; parse_index < reply_pattern.size(); ++parse_index) {
//         switch(reply_pattern[parse_index]) {
//             case '[':
//                 if (++parse_index < reply_pattern.size() && reply_pattern[parse_index] == '$') {
//                     result += reply_pattern.substr(last_index, parse_index++ - last_index - 1);
//                     if (parse_index > reply_pattern.size()) {
//                         last_index = parse_index - 2;
//                         break;
//                     }
//                     temp_num = reply_pattern[parse_index] - '0';
//                     if (reply_pattern[++parse_index] != ']') {
//                         last_index = parse_index-- - 2;
//                         break;
//                     }
//                     if (temp_num < 0 || temp_num > 9) throw(std::exception("解析错误：不合法的回复模板"));
//                     // ++temp_num cuz the prefix keywords will be count as params[0]
//                     if (++temp_num >= params.size()) {
//                         if (is_path) throw 0;
//                         result = Pattern::invalid_param_reply;
//                         return;
//                     }
//                     result += params[temp_num];
//                     last_index = parse_index + 1; 
//                 } else --parse_index;
//             break;
//             case '{':
//                 if (skip_json) break;
//                 if (++parse_index < reply_pattern.size() && reply_pattern[parse_index] == '$') {
//                     result += reply_pattern.substr(last_index, parse_index - last_index - 1);
//                     last_index = ++parse_index;
//                     while (parse_index < reply_pattern.size() && reply_pattern[parse_index] != '}') ++parse_index;
//                     if (parse_index >= reply_pattern.size()) {
//                         parse_index -= 2;
//                         break;
//                     }
//                     const rapidjson::Value &json_field = QBot_Utils::get_nested_json(root_json_obj, reply_pattern.substr(last_index, parse_index - last_index));
//                     if (json_field.IsString()) {
//                         result += json_field.GetString();
//                     } else if (json_field.IsInt()) {
//                         result += std::to_string(json_field.GetInt());
//                     } else if (json_field.IsDouble()) {
//                         result += std::to_string(json_field.GetDouble());
//                     } else throw(std::exception("解析错误：目标JSON域不为可以直接输出的类型（String、Number）"));
//                     last_index = parse_index + 1;
//                 } else --parse_index;
//             break;
//             case '\\':
//                 result += reply_pattern.substr(last_index, parse_index++ - last_index);
//                 last_index = parse_index;
//             default:
//             break;
//         }
//     }
//     if (last_index < reply_pattern.size()) result += reply_pattern.substr(last_index);
// }

void Pattern::set_new_url_pattern(const std::string& pattern, const size_t& param_count) {
    std::cmatch m;
    if (std::regex_match(pattern.c_str(), m, url_reg)) {
        req_url_domains[param_count] = m[1].str();
        req_url_hosts[param_count]   = m[3].str();
        std::string temp_path        = m[5].str();
        if (temp_path.empty()) temp_path = "/";
        size_t temp_index = 0;
        try {
            req_path_renderers[param_count].pre_render(temp_path, param_count, temp_index);
        } catch(const std::exception& err) {
            #ifdef DEBUG_VERSION
            cq::logging::error("错误", temp_path);
            #endif
            throw err;
        }
    } else throw std::exception("无法解析的 HTTP 链接，请注意需要准确填写协议头。");
}

void Pattern::get_reply_msg(const std::vector<std::string> &params, std::string &reply) const {
    int pattern_index = params.size() - 1;
    if (pattern_index < min_param_count) {
        reply = Pattern::invalid_param_reply;
        return;
    }
    
    rapidjson::Document root_json_obj;
    int status_code, client_index = -1;
    size_t random_index;
    switch(req_type) {
        case GET: {
            auto &target_path_renderer = req_path_renderers[pattern_index];
            auto &target_domain        = req_url_domains[pattern_index],
                 &target_host          = req_url_hosts[pattern_index];
            if (target_domain.empty()) {
                reply = Pattern::invalid_param_reply;
                return;
            }
            #ifdef TEST_CLIENT_POOL
            while (true) {
                client_pool.get_cli_index(client_index);
                #ifdef DEBUG_VERSION
                cq::logging::debug("client_index", std::to_string(client_index));
                #endif
                if (client_index == -1) std::this_thread::sleep_for(std::chrono::milliseconds(50));
                else break;
            }
            auto &cli = client_pool.get_cli_instance(client_index, target_domain);
            #else
            httplib::Client2 cli(target_domain.c_str());
            #endif
            httplib::Headers headers({
                {"Host", target_host.c_str()},
                {"Accept", "application/json;charset=utf-8"}
            });
            cli.set_connection_timeout(3, 0);
            cli.set_read_timeout(20, 0);
            std::string req_path;
            target_path_renderer.render(params, root_json_obj, req_path);
            try {
                const auto &res = cli.Get(req_path.c_str(), headers);
                #ifdef TEST_CLIENT_POOL
                client_pool.put_cli_back(client_index);
                client_index = -1;
                #endif
                if (res) {
                    status_code = res->status;
                    if (root_json_obj.Parse(res->body.c_str()).HasParseError()) {
                        cq::logging::warning("非JSON请求内容", res->body);
                        throw std::exception("请求返回内容不是合法的JSON数据结构。");
                    }
                } else {
                    // Network issue, try again!
                    if (network_issue_report.empty()) throw std::exception("网络请求未被回复");
                    reply += Pattern::network_issue_report;
                    cq::logging::warning("警告", "网络请求未被回复");
                    return;
                }
            } catch(const std::exception& err) {
                #ifdef TEST_CLIENT_POOL
                if (client_index != -1) client_pool.put_cli_back(client_index);
                #endif
                throw err;
            }
        }
        break;
        case POST:
            throw std::exception("POST 方法尚未实现，不允许使用");
        break;
        case AUTO_REPLY:
            // do nothing about document
            // get the first availble status_code as chosen
            if (!reply_patterns.empty()) status_code = reply_patterns.begin()->first;
        break;
        default:
            throw std::exception("不合法的回复解析模式，请检查数据文件或设置是否错误");
    }
    if (reply_patterns.empty()) throw("没有可用的回复模板"); // return std::string("");
    auto reply_pattern_iter = reply_pattern_renderers.find(status_code);
    if (reply_pattern_iter == reply_pattern_renderers.end()) throw std::exception("没有可用的回复模板");
    
    const auto &reply_render_vector = reply_pattern_iter->second;
    
    if (reply_render_vector.size() == 0) throw std::exception("没有可用的回复模板");
    
    // 随机选择回复模板
    random_index = clock() % reply_render_vector.size();
    const auto reply_renderer = reply_render_vector[random_index];
    reply_renderer.render(params, root_json_obj, reply);
    reply += Pattern::trailling_content;
}