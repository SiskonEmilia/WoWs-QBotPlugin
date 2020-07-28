#ifndef QBOT_PATTERN_HPP
#define QBOT_PATTERN_HPP
#include "Global.hpp"
#include <vector>
#include <map>

/**
* Pattern 类代表了一个 关键词-HTTP请求组合
* 数据域表：
* - is_enable        ：规则是否启用
* - prefix_keyword   ：触发 pattern 的前缀关键字
* - req_type         ：pattern 所使用的请求类型
* - req_url_pattern  ：HTTP 请求的模式或域名
* - reply_patterns   ：回复模板表
* *** 以下部分暂未计划实现 ***
* - post_body_pattern：当使用 POST 请求时的 request body
*/
typedef struct Pattern {
    bool is_enable;
    std::string prefix_keyword;
    HTTP_REQUEST_TYPE req_type;
    /** 关于 req_url_pattern 的语法说明
    * 示例：
    * http(s)://example.com?fieldname1=[$paramater_index]&fieldname2=[$paramater_index]&...
    * 在 [paramater_index] 处，您应该填写其在指令中的参数序号，如指令为
    * “搜船 AB BC CD”
    * 则 AB 对应 0，BC 对应 1，CD 对应 2
    * 如果 req_url_pattern 为 http://example.com?test1=[$2]&test2=[$1]
    * 则实际请求 URL 为 http://example.com?test1=BC&test2=CD
    */    
    std::string req_url_pattern;
    /** 关于 reply_pattern 的语法说明
     * 示例：
     * 查询结果[$paramater_index], {$JSON_field.nestedJSON_field}
     * reply_pattern 中允许同时出现两种参数，其中 [$] 是用户之前输出的参数，{$} 是 HTTP 请求
     * 返回的 JSON 的域变量，当你要访问 JSONObj[abc][bcd] 时，写作 {$abc.bcd}。因此，我们不允
     * 许参数内出现 "." 符号，请务必注意。
    */
    std::map<int, std::vector<std::string>*> reply_patterns;
    Pattern(const bool is_enable, const char* prefix_keyword, const int req_type, const char* req_url_pattern);
    ~Pattern();
    // std::string post_body_pattern
} Pattern;

#endif