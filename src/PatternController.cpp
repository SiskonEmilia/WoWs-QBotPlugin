#include "PatternController.hpp"

#include <fstream>
#include <algorithm>
#include <exception>
#include <cstdio>
#include <regex>

#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

PatternController* PatternController::single_instance = nullptr;

void PatternController::load_from_file() {
    #ifdef DEBUG_VERSION
    cq::logging::debug("DEBUG info", "PatternController::load_from_file() called.");
    #endif
    FILE* json_file_ptr = fopen("patterns_config.json", "rb");
    if (json_file_ptr == nullptr) {
        // file not exists
        cq::logging::warning("注意", "数据 JSON 文件不存在或无文件读写权限，无法正常读取数据。");
    } else {
        #ifdef DEBUG_VERSION
        cq::logging::debug("进程", "已打开 JSON 文件，尝试读取设置信息");
        #endif
        char readBuffer[65536]; // 文件读入缓冲区
        rapidjson::FileReadStream json_is(json_file_ptr, readBuffer, sizeof(readBuffer));

        rapidjson::Document document;
        try {
            if (document.ParseStream(json_is).HasParseError()) throw std::exception("CANNOT PARSE");
        } catch (std::exception err) {
            cq::logging::error("错误", "数据 JSON 文件已经损坏。");
            throw(err);
        }

        size_t temp_param_count, temp_index;
        int status_code;
        // 以下为解析 JSON，加载设置的内容
        rapidjson::Value &json_patterns = document["patterns"];
        if (json_patterns.IsArray()) {
            for (auto &pattern : json_patterns.GetArray()) {
                rapidjson::Value &json_is_enable = pattern["is_enable"], 
                    &json_prefix_keyword = pattern["prefix_keyword"],
                    &json_req_type = pattern["req_type"],
                    &json_req_url_patterns = pattern["req_url_patterns"],
                    &json_reply_patterns = pattern["reply_patterns"];
                if (json_is_enable.IsBool() && json_prefix_keyword.IsString() &&
                    json_req_type.IsInt() && json_req_url_patterns.IsArray() &&
                    json_reply_patterns.IsArray()) {
                    // Try to insert the new pattern
                    auto &new_pattern_iter = patterns.insert({json_prefix_keyword.GetString(), Pattern(json_is_enable.GetBool(),
                        json_prefix_keyword.GetString(),
                        json_req_type.GetInt())});
                    if (!new_pattern_iter.second) {
                        reset();
                        fclose(json_file_ptr);
                        throw(std::exception("配置错误：每个关键词仅能配置一个模式"));
                    }
                    auto &new_pattern = (*new_pattern_iter.first).second;
                    for (auto &req_url_pattern_obj: json_req_url_patterns.GetArray()) {
                        if (req_url_pattern_obj.IsObject()) {
                            rapidjson::Value &json_req_url_pattern = req_url_pattern_obj["req_url_pattern"],
                                &json_param_count = req_url_pattern_obj["param_count"];
                            if (json_param_count.IsInt()) {
                                temp_param_count = json_param_count.GetInt();
                                if (temp_param_count <= 10) {
                                    new_pattern.min_param_count = std::min(new_pattern.min_param_count, temp_param_count);
                                    if (new_pattern.req_type == HTTP_REQUEST_TYPE::AUTO_REPLY) break;
                                    if (json_req_url_pattern.IsString()) {
                                        if (new_pattern.req_url_patterns[temp_param_count].empty()) {
                                            try {
                                                new_pattern.set_new_url_pattern(json_req_url_pattern.GetString(), temp_param_count);
                                            } catch(const std::exception &err) {
                                                reset();
                                                fclose(json_file_ptr);
                                                throw err;
                                            }
                                        } else {
                                            reset();
                                            fclose(json_file_ptr);
                                            throw(std::exception("配置错误：每个 param_count 仅能对应唯一的请求模式"));
                                        }
                                    } else {
                                        reset();
                                        fclose(json_file_ptr);
                                        throw(std::exception("JSON数据错误：req_url_patterns域内req_url_pattern不为字符串"));
                                    }
                                } else {
                                    reset();
                                    fclose(json_file_ptr);
                                    throw(std::exception("JSON数据错误：req_url_patterns域内param_count不在[0,10]范围内"));
                                }
                            } else {
                                reset();
                                fclose(json_file_ptr);
                                throw(std::exception("JSON数据错误：req_url_patterns域内param_count不为整形"));
                            }
                        } else {
                            reset();
                            fclose(json_file_ptr);
                            throw(std::exception("JSON数据类型错误：req_url_patterns域内Object"));
                        }
                    }
                    for (auto &reply_pattern: json_reply_patterns.GetArray()) {
                        if (reply_pattern.IsObject()) {
                            rapidjson::Value &json_status_code = reply_pattern["status_code"],
                            &json_random_reply_patterns = reply_pattern["random_reply_patterns"];
                            if (json_status_code.IsInt() && json_random_reply_patterns.IsArray()) {
                                status_code = json_status_code.GetInt();
                                if (new_pattern.reply_patterns.count(status_code) == 0) {
                                    new_pattern.reply_patterns.insert({status_code, std::vector<std::string>()});
                                } else {
                                    reset();
                                    fclose(json_file_ptr);
                                    throw(std::exception("JSON数据类型错误：重复定义单一状态码的回复模式组"));
                                }
                                    
                                // 读取并保存所有的回复模板
                                auto &reply_patterns_vector = new_pattern.reply_patterns[status_code];
                                for (auto &random_reply_pattern: json_random_reply_patterns.GetArray()) {
                                    if (random_reply_pattern.IsString()) {
                                        reply_patterns_vector.push_back(random_reply_pattern.GetString());
                                    } else {
                                        reset();
                                        fclose(json_file_ptr);
                                        throw(std::exception("JSON数据类型错误：random_reply_pattern内应为string"));
                                    }
                                }

                                // 预渲染所有的回复模板
                                new_pattern.reply_pattern_renderers.insert({status_code, std::vector<RendererObject>(reply_patterns_vector.size())});
                                auto &reply_renderers = new_pattern.reply_pattern_renderers[status_code];
                                for (size_t i = 0; i < reply_patterns_vector.size(); ++i) {
                                    temp_index = 0;
                                    try {
                                        reply_renderers[i].pre_render(reply_patterns_vector[i], new_pattern.min_param_count, temp_index, true);
                                    } catch (const std::exception &err) {
                                        #ifdef DEBUG_VERSION
                                        cq::logging::error("错误", reply_patterns_vector[i]);
                                        #endif
                                        reset();
                                        throw err;
                                    }    
                                }
                                
                            } else {
                                reset();
                                fclose(json_file_ptr);
                                throw(std::exception("JSON数据类型错误：reply_patterns域Object内"));
                            }
                        } else {
                            reset();
                            fclose(json_file_ptr);
                            throw(std::exception("JSON数据类型错误：reply_patterns域内Object"));
                        }
                    }
                } else {
                    reset();
                    fclose(json_file_ptr);
                    throw(std::exception("JSON数据类型错误：pattern域内"));
                }
            }
        } else {
            fclose(json_file_ptr);
            throw(std::exception("JSON文件格式错误：patterns域"));
        }

        rapidjson::Value &json_invalid_param_reply = document["invalid_param_reply"];
        if (json_invalid_param_reply.IsString()) {
            Pattern::invalid_param_reply = json_invalid_param_reply.GetString();
        } else {
            fclose(json_file_ptr);
            throw(std::exception("JSON文件格式错误：invalid_param_reply域"));
        }

        rapidjson::Value &json_trailling_content = document["trailling_content"];
        if (json_trailling_content.IsString()) {
            Pattern::trailling_content = json_trailling_content.GetString();
        } else {
            fclose(json_file_ptr);
            throw(std::exception("JSON文件格式错误：trailling_content域"));
        }

        rapidjson::Value &json_i_need_help = document["i_need_help"];
        if (json_i_need_help.IsString()) {
            Pattern::i_need_help = json_i_need_help.GetString();
        } else {
            cq::logging::warning("JSON数据警告", "未设置呼救字段，出现问题将不会在群内呼救");
        }

        rapidjson::Value &json_param_splitor = document["param_splitor"];
        if (json_param_splitor.IsString()) {
            std::string param_splitor_str(json_param_splitor.GetString());
            if (param_splitor_str.size() != 1) {
                cq::logging::warning("JSON数据警告", "未设置或错误设置文件分隔符，已恢复为默认的空格");
            } else {
                Pattern::param_splitor = param_splitor_str[0];
            }
        } else {
            cq::logging::warning("JSON数据警告", "未设置或错误设置文件分隔符，已恢复为默认的空格");
        }
        #ifdef DEBUG_VERSION
        cq::logging::debug("进程", "解析设置完成，尝试关闭文件指针");
        #endif
        fclose(json_file_ptr);
    }
}

void PatternController::save_to_file() {
    #ifdef DEBUG_VERSION
    cq::logging::debug("DEBUG info", "PatternController::save_to_file() called.");
    cq::logging::debug("DEBUG info", "This function is not defined.");
    #endif
}

PatternController::PatternController() {
    #ifdef DEBUG_VERSION
    cq::logging::debug("DEBUG info", "PatternController::PatternController() called.");
    cq::logging::debug("DEBUG info", "This function is not defined.");
    #endif
}

PatternController::~PatternController() {
    #ifdef DEBUG_VERSION
    cq::logging::debug("DEBUG info", "PatternController::~PatternController() called.");
    #endif
    save_to_file();
}

PatternController* PatternController::get_instance() {
    #ifdef DEBUG_VERSION
    cq::logging::debug("DEBUG info", "PatternController::get_instance() called.");
    cq::logging::debug("DEBUG info", "This function is not defined.");
    #endif
    if (single_instance == nullptr) single_instance = new PatternController();
    return single_instance;
}

void PatternController::reset() {
    patterns.clear();
}

std::map<std::string, Pattern>::iterator PatternController::find_pattern(std::string key) {
    return patterns.find(key);
}


std::map<std::string, Pattern>::iterator PatternController::get_end() {
    return patterns.end();
}