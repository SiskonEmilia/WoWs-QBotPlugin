#ifndef QBOT_UTILS
#define QBOT_UTILS
#include "Global.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <exception>
#include <rapidjson/document.h>

namespace QBot_Utils {
    static std::string bot_mentioned_key;

    inline void split_group_msg(const std::string &input_str, const char &splitor, 
        std::vector<std::string> &result_container) {
        std::istringstream is(input_str.substr(bot_mentioned_key.size()));
        std::string fragment_str;

        while(std::getline(is, fragment_str, splitor)) {
            if (!fragment_str.empty())
                result_container.push_back(fragment_str);
        }
    }

    inline void split_private_msg(const std::string &input_str, const char &splitor, 
        std::vector<std::string> &result_container) {
        std::istringstream is(input_str);
        std::string fragment_str;

        while(std::getline(is, fragment_str, splitor)) {
            if (!fragment_str.empty())
                result_container.push_back(fragment_str);
        }
    }

    inline void set_mentioned_key (const std::string &bot_id) {
        bot_mentioned_key = "[CQ:at,qq=";
        bot_mentioned_key += bot_id;
        bot_mentioned_key += "]";
    }

    inline bool is_mentioned(const std::string &message) {
        if (message.size() < bot_mentioned_key.size()) return false;
        return message.substr(0, bot_mentioned_key.size()) == bot_mentioned_key;
    }

    inline const rapidjson::Value& get_nested_json(const rapidjson::Value &root_obj, const std::string &keys) {
        if (root_obj.IsObject()) {
            size_t temp_index = keys.find('.');
            if (temp_index == std::string::npos) {
                return root_obj[keys.c_str()];
            } else {
                return get_nested_json(root_obj[keys.substr(0, temp_index).c_str()], keys.substr(temp_index + 1));
            }
        } else {
            throw(std::exception("JSON 嵌套解析错误"));
        }
        
    }
}

#endif