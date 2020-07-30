#ifndef RENDERER_HPP
#define RENDERER_HPP
#include <rapidjson/document.h>
#include <string>
#include <vector>

class RendererObject {
private:
    std::vector<std::string> literal_substrings;
    std::vector<RendererObject> renderer_objs;
protected:
    std::string json_field;
    size_t param_index;
public:
    typedef enum RendererType {
        ROOT, ARRAY, OBJECT, PARAMETER
    } RendererType;
    RendererType obj_type;
    /**
     * pre_render 函数说明
     * - pattern:      const std::string&: 待解析的模式串
     * - params_count: const int&:         解析允许的最大参数个数
     * - start_index:  int                 开始解析的模式串位置
     * - return:       int                 结束解析的模式串位置（即解析了 [start_index, return)）     
    */
    RendererObject(bool isArray = false);
    RendererObject(const size_t &param_index);
    RendererObject(const std::string &json_field);
    void pre_render(const std::string &pattern, const size_t& params_count, size_t& start_index, bool json_enable = false);
    void render(const std::vector<std::string> &params, const rapidjson::Value &root_json_obj, std::string &result) const;
};

#endif