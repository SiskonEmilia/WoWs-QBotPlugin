#include "Renderer.hpp"
#include "QBotUtils.hpp"
#include <exception>

RendererObject::RendererObject(bool isArray) : param_index(0), json_field("") {
    if (isArray) obj_type = ARRAY;
    else obj_type = ROOT;
}

RendererObject::RendererObject(const size_t &param_index) : obj_type(PARAMETER), param_index(param_index), json_field("") {}

RendererObject::RendererObject(const std::string &json_field) : obj_type(OBJECT), param_index(0), json_field(json_field) {}

void RendererObject::pre_render(const std::string &pattern, const size_t& params_count, size_t& start_index, bool json_enable) {
    size_t literal_top_index = 0;
    literal_substrings.push_back("");
    size_t parse_index = 0, temp_param_index = 0;
    for (parse_index = start_index; parse_index < pattern.size(); ++parse_index) {
        switch(pattern[parse_index]) {
            case '[':
                if (++parse_index < pattern.size()) {
                    if (pattern[parse_index] == '$') {
                        // 记录之前的字面值字符串
                        if (literal_top_index < renderer_objs.size()) {
                            ++literal_top_index;
                            literal_substrings.push_back("");
                        }
                        literal_substrings[literal_top_index] += pattern.substr(start_index, parse_index - start_index - 1);
                        if (++parse_index >= pattern.size()) throw std::exception("模式预解析错误：未闭合的 parameter 标签");
                        temp_param_index = pattern[parse_index] - '0';
                        if (temp_param_index > params_count) throw std::exception("模式预解析错误：parameter_index 超过允许范围");
                        if (++parse_index >= pattern.size() || pattern[parse_index] != ']')
                            throw std::exception("模式预解析错误：未闭合的 parameter 标签或 parameter_index 超过允许范围");
                        renderer_objs.push_back(RendererObject(temp_param_index));
                        renderer_objs[literal_top_index].obj_type = PARAMETER;
                        start_index = parse_index + 1;
                    } else --parse_index;
                }
            break;
            case '{':
                if (json_enable && ++parse_index < pattern.size()) {
                    if (pattern[parse_index] == '$') { 
                        // 记录之前的字面值字符串
                        if (literal_top_index < renderer_objs.size()) {
                            ++literal_top_index;
                            literal_substrings.push_back("");
                        }
                        literal_substrings[literal_top_index] += pattern.substr(start_index, parse_index - start_index - 1);
                        start_index = parse_index + 1;
                        renderer_objs.push_back(RendererObject(std::string("")));

                        // 开始记录 Object 的 json_field
                        auto &target_obj = renderer_objs[literal_top_index];
                        while (++parse_index < pattern.size()) {
                            if (pattern[parse_index] == '\\') {
                                target_obj.json_field += pattern.substr(start_index, parse_index++ - start_index);
                                start_index = parse_index + 1;
                            } else if (pattern[parse_index] == '}') {
                                target_obj.json_field += pattern.substr(start_index, parse_index - start_index);
                                start_index = parse_index + 1;
                                break;
                            }
                        }
                        target_obj.obj_type = OBJECT;
                        if (parse_index >= pattern.size()) throw std::exception("模式预解析错误：未闭合的 JSON field 标签");
                    } else --parse_index;
                }
            break;
            case '<':
                if (json_enable && ++parse_index < pattern.size()) {
                    if (pattern[parse_index] == '$') {
                        // 记录之前的字面值字符串
                        if (literal_top_index < renderer_objs.size()) {
                            ++literal_top_index;
                            literal_substrings.push_back("");
                        }
                        literal_substrings[literal_top_index] += pattern.substr(start_index, parse_index - start_index - 1);
                        start_index = ++parse_index;
                        renderer_objs.push_back(RendererObject(true));
                        
                        // 开始记录子 Array 的 json_field
                        auto &target_obj = renderer_objs[literal_top_index];
                        while (parse_index < pattern.size()) {
                            if (pattern[parse_index] == '\\') {
                                target_obj.json_field += pattern.substr(start_index, parse_index++ - start_index);
                                start_index = parse_index++;
                            } else if (pattern[parse_index] == '>') {
                                target_obj.json_field += pattern.substr(start_index, parse_index - start_index);
                                start_index = ++parse_index;
                                break;
                            } else ++parse_index;
                        }
                        #ifdef DEBUG_VERSION
                        cq::logging::warning("注意", target_obj.json_field + "数组渲染体被定义");
                        cq::logging::warning("注意", pattern);
                        #endif
                        if (parse_index >= pattern.size()) throw std::exception("模式预解析错误：未闭合的 JSON List 起始标签");
                        target_obj.obj_type = ARRAY;
                        target_obj.pre_render(pattern, params_count, start_index, true);
                        parse_index = start_index - 1;
                    } else if (pattern[parse_index] == '/') {
                        if (++parse_index < pattern.size() && pattern[parse_index] == '>') {
                            if (obj_type == ARRAY) {
                                // 记录最后的字符串，子模式解析结束
                                if (literal_top_index < renderer_objs.size()) {
                                    ++literal_top_index;
                                    literal_substrings.push_back("");
                                }
                                literal_substrings[literal_top_index] += pattern.substr(start_index, parse_index - start_index - 2);
                                start_index = parse_index + 1;
                                return;
                            } else throw std::exception("模式预解析错误：未配对的 JSON List 结束标签");
                        } else throw std::exception("模式预解析错误：未闭合的 JSON List 结束标签");
                    } else --parse_index;
                }
            break;
            case '\\':
                // do not parse the next character
                if (literal_top_index < renderer_objs.size()) {
                    ++literal_top_index;
                    literal_substrings.push_back("");
                }
                literal_substrings[literal_top_index] += pattern.substr(start_index, parse_index - start_index);
                start_index = ++parse_index;
                break;
            default:
            break;
        }
    }
    if (start_index < pattern.size()) {
        if (literal_top_index < renderer_objs.size()) {
            ++literal_top_index;
            literal_substrings.push_back("");
        }
        literal_substrings[literal_top_index] += pattern.substr(start_index, pattern.size() - start_index);
    }
}

void RendererObject::render(const std::vector<std::string> &params, const rapidjson::Value &root_json_obj, std::string &result_container) const{
    size_t sub_pattern_index = 0, sub_array_index = 0;
    for(; sub_pattern_index < renderer_objs.size(); ++sub_pattern_index) {
        result_container += literal_substrings[sub_pattern_index];
        const auto& sub_pattern = renderer_objs[sub_pattern_index];
        switch(sub_pattern.obj_type) {
            case ARRAY: { 
                #ifdef DEBUG_VERSION
                cq::logging::warning("Parsing array", "ENTER");
                #endif
                try {
                    const auto& json_target_array = QBot_Utils::get_nested_json(root_json_obj, sub_pattern.json_field);
                    if (json_target_array.IsArray()) {
                        const auto& target_array = json_target_array.GetArray();
                        if (target_array.Size() > 0) {
                            const auto &first_ele = target_array[0];
                            if (first_ele.IsObject()) {
                                for (const auto& target_object : target_array) {
                                    sub_pattern.render(params, target_object, result_container);
                                }
                            } else if (first_ele.IsString()) {
                                for (const auto& target_object : target_array) {
                                    result_container += target_object.GetString();
                                    sub_pattern.render(params, target_object, result_container);
                                }
                            } else if (first_ele.IsDouble()) {
                                for (const auto& target_object : target_array) {
                                    result_container += std::to_string(target_object.GetDouble());
                                    sub_pattern.render(params, target_object, result_container);
                                }
                            } else if (first_ele.IsInt()) {
                                for (const auto& target_object : target_array) {
                                    result_container += std::to_string(target_object.GetInt());
                                    sub_pattern.render(params, target_object, result_container);
                                }
                            } else throw std::exception("渲染字符串时发生错误：不支持的数组成员（仅支持Object、数值）");
                        } 
                        #ifdef DEBUG_VERSION
                        else {
                            cq::logging::warning("Parsing array", "Nothing to ouput");
                        }
                        #endif
                    } else { 
                        #ifdef PRINT_NULL
                        result_container += "\nNULL\n";
                        #else
                        throw std::exception("渲染字符串时发生错误：找不到模板中指定的数组域");
                        #endif
                    }
                } catch(const std::string& null_err) {
                    #ifdef PRINT_NULL
                    result_container += "\nNULL\n";
                    #endif
                }
            }
            break;
            case OBJECT:{
                const auto& target_object = QBot_Utils::get_nested_json(root_json_obj, sub_pattern.json_field);
                if (target_object.IsString()) {
                    result_container += target_object.GetString();
                } else if (target_object.IsInt()) {
                    result_container += std::to_string(target_object.GetInt());
                } else if (target_object.IsDouble()) {
                    result_container += std::to_string(target_object.GetDouble());
                }
                #ifdef PRINT_NULL 
                else if (target_object.IsNull()) {
                    result_container += "NULL";
                }
                #endif
                else throw std::exception("渲染字符串时发生错误：找不到模板中指定的字符串/数字域");
            }
            break;
            case PARAMETER:
                result_container += params[sub_pattern.param_index + 1];
            break;
            default:
                throw std::exception("渲染字符串时发生错误：非法的 RendererObject::obj_type 类型");
        }
    }
    if (sub_pattern_index < literal_substrings.size()) result_container += literal_substrings[sub_pattern_index];
}