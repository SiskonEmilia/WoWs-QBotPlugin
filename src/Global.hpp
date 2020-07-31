/*
* 该头文件包含了一些插件内的全局定义类型
*/
#ifndef QBOT_GLOBAL_HPP
#define QBOT_GLOBAL_HPP
#include <string>
#include <cqcppsdk/cqcppsdk.h>

// comment if release
// #define DEBUG_VERSION
#define PRINT_NULL
// #define GUI_TEST_SET

#define MAX_AVALIBEL_THREAD 128

typedef enum HTTP_REQUEST_TYPE {
    GET, 
    POST,
    AUTO_REPLY
} HTTP_REQUEST_TYPE;

// 用于复读计数的结构体
typedef struct MessageCount {
    short count;
    std::string message;
} MessageCount;


#endif