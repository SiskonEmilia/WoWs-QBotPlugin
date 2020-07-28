/*
* 该头文件包含了一些插件内的全局定义类型
*/
#ifndef QBOT_GLOBAL_HPP
#define QBOT_GLOBAL_HPP
#include <string>
#include <cqcppsdk/cqcppsdk.h>

typedef enum HTTP_REQUEST_TYPE {
    GET, 
    POST,
    AURO_REPLY
} HTTP_REQUEST_TYPE;

// 用于复读计数的结构体
typedef struct MessageCount {
    short count;
    std::string message;
} MessageCount;


#endif