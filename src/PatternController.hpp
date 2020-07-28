#ifndef QBOT_PATTERNCONTROLLER_HPP
#define QBOT_PATTERNCONTROLLER_HPP

/*
* 本文件包含了 PatternController 单例类的定义
* 其负责保存和管理所有设定数据、运行时数据
* 并在设置改变时负责对数据进行持久化，在再次运行时恢复上次的设置
*/

#include "Global.hpp"
#include "Pattern.hpp"
#include <map>

// 保存配置文件的路径
#define CONFIG_FILE_PATH "./WoWsQBotConfig.ini"

class PatternController {
private:
    static PatternController *single_instance;
    PatternController();
    std::map<std::string, Pattern*> patterns;
public:
    static PatternController* get_instance();
    void save_to_file();
    void load_from_file();
    void reset();
    ~PatternController();
    std::map<std::string, Pattern*>::iterator find_pattern(std::string key);
    std::map<std::string, Pattern*>::iterator get_end();
};

#endif