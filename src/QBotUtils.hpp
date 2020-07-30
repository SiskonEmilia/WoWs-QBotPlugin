#ifndef QBOT_UTILS
#define QBOT_UTILS
#include "Global.hpp"
#include "Pattern.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <exception>
#include <rapidjson/document.h>

namespace QBot_Utils {

    inline void split_group_msg(const std::string &input_str, const char &splitor, 
        std::vector<std::string> &result_container) {
        int prefix_empty = 0;
        std::string input_str_without_at(input_str.substr(Pattern::bot_mentioned_key.size()));
        for (const auto &ch: input_str_without_at) {
            if (ch == ' ') ++prefix_empty;
            else break;
        }
        std::istringstream is(input_str_without_at.substr(prefix_empty));
        std::string fragment_str;

        while(std::getline(is, fragment_str, splitor)) {
            if (!fragment_str.empty())
                result_container.push_back(fragment_str);
        }
    }

    inline void split_private_msg(const std::string &input_str, const char &splitor, 
        std::vector<std::string> &result_container) {
        int prefix_empty = 0;
        for (const auto &ch: input_str) {
            if (ch == ' ') ++prefix_empty;
            else break;
        }
        std::istringstream is(input_str.substr(prefix_empty));
        std::string fragment_str;

        while(std::getline(is, fragment_str, splitor)) {
            if (!fragment_str.empty())
                result_container.push_back(fragment_str);
        }
    }

    inline void set_mentioned_key (const std::string &bot_id) {
        Pattern::bot_mentioned_key = CQAT_CODE_PREFIX;
        Pattern::bot_mentioned_key += bot_id;
        Pattern::bot_mentioned_key += "]";
    }

    inline std::string get_mention_key (const std::string &user_id) {
        std::string mention_key = CQAT_CODE_PREFIX;
        mention_key += user_id;
        mention_key += "]\n";
        return mention_key;
    }

    inline bool is_mentioned(const std::string &message) {
        if (message.size() < Pattern::bot_mentioned_key.size()) return false;
        return message.substr(0, Pattern::bot_mentioned_key.size()) == Pattern::bot_mentioned_key;
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
            #ifdef PRINT_NULL
            throw std::string("NULL");
            #else
            throw std::exception("JSON 嵌套解析失败，请检查您的 JSON 结构");
            #endif
        }
        
    }
}

#endif