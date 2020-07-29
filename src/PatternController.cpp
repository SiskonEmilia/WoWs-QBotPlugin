#include "PatternController.hpp"
#include <fstream>
#include <exception>
#include <cstdio>
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

        size_t temp_param_count;
        // 以下为解析 JSON，加载设置的内容
        rapidjson::Value &json_patterns = document["patterns"];
        if (json_patterns.IsArray()) {
            Pattern *new_pattern_ptr;
            std::vector<std::string> *new_reply_patterns_ptr;
            for (auto &pattern : json_patterns.GetArray()) {
                rapidjson::Value &json_is_enable = pattern["is_enable"], 
                    &json_prefix_keyword = pattern["prefix_keyword"],
                    &json_req_type = pattern["req_type"],
                    &json_req_url_patterns = pattern["req_url_patterns"],
                    &json_reply_patterns = pattern["reply_patterns"];
                if (json_is_enable.IsBool() && json_prefix_keyword.IsString() &&
                    json_req_type.IsInt() && json_req_url_patterns.IsArray() &&
                    json_reply_patterns.IsArray()) {
                    new_pattern_ptr = new Pattern(json_is_enable.GetBool(),
                        json_prefix_keyword.GetString(),
                        json_req_type.GetInt());
                    for (auto &req_url_pattern_obj: json_req_url_patterns.GetArray()) {
                        if (req_url_pattern_obj.IsObject()) {
                            rapidjson::Value &json_req_url_pattern = req_url_pattern_obj["req_url_pattern"],
                                &json_param_count = req_url_pattern_obj["param_count"];
                            if (json_req_url_pattern.IsString() && json_param_count.IsInt()) {
                                temp_param_count = json_param_count.GetInt();
                                if (temp_param_count < 10) {
                                    new_pattern_ptr->req_url_patterns[temp_param_count] = json_req_url_pattern.GetString();
                                    if (!Pattern::verify_pattern(new_pattern_ptr->req_url_patterns[temp_param_count], temp_param_count)) {
                                        delete new_pattern_ptr;
                                        reset();
                                        fclose(json_file_ptr);
                                        throw(std::exception("JSON数据错误：req_url_patterns域内url模式不合法"));
                                    }
                                } else {
                                    delete new_pattern_ptr;
                                    reset();
                                    fclose(json_file_ptr);
                                    throw(std::exception("JSON数据错误：req_url_patterns域内param_count不在[0,10]范围内"));
                                }
                            }
                        } else {
                            delete new_pattern_ptr;
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
                                new_reply_patterns_ptr = new std::vector<std::string>();
                                for (auto &random_reply_pattern: json_random_reply_patterns.GetArray()) {
                                    if (random_reply_pattern.IsString()) {
                                        new_reply_patterns_ptr->push_back(random_reply_pattern.GetString());
                                    } else { 
                                        delete new_reply_patterns_ptr;
                                        delete new_pattern_ptr;
                                        reset();
                                        fclose(json_file_ptr);
                                        throw(std::exception("JSON数据类型错误：random_reply_patterns内string"));
                                    }
                                }
                                new_pattern_ptr->reply_patterns[json_status_code.GetInt()] = new_reply_patterns_ptr;
                            } else {
                                delete new_pattern_ptr;
                                reset();
                                fclose(json_file_ptr);
                                throw(std::exception("JSON数据类型错误：reply_patterns域Object内"));
                            }
                        } else {
                            delete new_pattern_ptr;
                            reset();
                            fclose(json_file_ptr);
                            throw(std::exception("JSON数据类型错误：reply_patterns域内Object"));
                        }
                    }
                    patterns[std::string(json_prefix_keyword.GetString())] = new_pattern_ptr;
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
    for (auto &pattern :patterns) {
        delete pattern.second;
    }
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
    for (auto &pattern :patterns) {
        delete pattern.second;
    }
    patterns.clear();
}

std::map<std::string, Pattern*>::iterator PatternController::find_pattern(std::string key) {
    return patterns.find(key);
}


std::map<std::string, Pattern*>::iterator PatternController::get_end() {
    return patterns.end();
}