#include <iostream>
#include <set>
#include <sstream>
#include <exception>
#include <QApplication>

#include "Global.hpp"
#include "PatternController.hpp"
#include "mainwindow.h"

using namespace cq;
using namespace std;
using Message = cq::message::Message;
using MessageSegment = cq::message::MessageSegment;

PatternController *pc_instance = nullptr;
QApplication *a = nullptr;
MainWindow *w = nullptr;
int tmp_num = 0;
char **tmp_argv = new char*[0];

CQ_INIT {
    on_enable([] { 
        logging::info("启动", "WoWsQBot插件已启用【您正在使用测试版】");
        logging::info("提示", "测试版的性能和稳定性均不能保证，请慎重使用");
        logging::info("提示", "本插件 Github 地址：https://github.com/SiskonEmilia/WoWs-QBotPlugin");
        logging::info("提示", "如有发生严重 BUG，请在 Github 页面提交 issue");
        logging::info("测试", "测试：文件读取");
        try {
            pc_instance = PatternController::get_instance();
            pc_instance->load_from_file();
            logging::info_success("测试", "测试：文件读取成功");
            pc_instance->save_to_file();
            logging::info_success("测试", "测试：文件保存成功");
        }
        catch(const std::exception& e) {
            logging::error("错误", "文件存取时发生错误");
            logging::error("错误", e.what());
        }

        logging::info("测试", "测试：打开设置面板");
        try {
            if (a == nullptr) a = new QApplication(tmp_num, tmp_argv);
            if (w == nullptr) w = new MainWindow();
            w->show();
            a->exec();
            logging::info_success("测试", "测试：文件保存成功");
        } catch (const std::exception& e) {
            logging::error("错误", "GUI测试时发生错误");
            logging::error("错误", e.what());
        }
    });

    on_private_message([](const PrivateMessageEvent &event) {
        try {
            auto pattern = pc_instance->find_pattern(event.message);
            if (pattern == pc_instance->get_end()) {
                send_message(event.target, "您没有触发任何关键词~");
            } else {
                
            }
        } catch (ApiError &err) {
            logging::warning("私聊", "私聊消息复读失败, 错误码: " + to_string(err.code));
        }
    });

    on_message([](const MessageEvent &event) {
        logging::debug("消息", "收到消息: " + event.message + "\n实际类型: " + typeid(event).name());
    });

    on_group_message([](const GroupMessageEvent &event) {
        static const set<int64_t> ENABLED_GROUPS = {123456, 123457};
        if (ENABLED_GROUPS.count(event.group_id) == 0) return; // 不在启用的群中, 忽略

        try {
            send_message(event.target, event.message); // 复读
            auto mem_list = get_group_member_list(event.group_id); // 获取群成员列表
            string msg;
            for (auto i = 0; i < min(10, static_cast<int>(mem_list.size())); i++) {
                msg += "昵称: " + mem_list[i].nickname + "\n"; // 拼接前十个成员的昵称
            }
            send_group_message(event.group_id, msg); // 发送群消息
        } catch (ApiError &) { // 忽略发送失败
        }
        if (event.is_anonymous()) {
            logging::info("群聊", "消息是匿名消息, 匿名昵称: " + event.anonymous.name);
        }
        event.block(); // 阻止当前事件传递到下一个插件
    });

    on_group_upload([](const auto &event) { // 可以使用 auto 自动推断类型
        stringstream ss;
        ss << "您上传了一个文件, 文件名: " << event.file.name << ", 大小(字节): " << event.file.size;
        try {
            send_message(event.target, ss.str());
        } catch (ApiError &) {
        }
    });

    on_disable([]() {
        if (w != nullptr) {
            delete w;
            w = nullptr;
        }
        if (a != nullptr) {
            delete a;
            a = nullptr;
        }
    });

    // on_coolq_exit([]() {
    //     if (w != nullptr) {
    //         delete w;
    //         w = nullptr;
    //     }
    //     if (a != nullptr) {
    //         delete a;
    //         a = nullptr;
    //     }
    // });
}

CQ_MENU(menu_demo_1) {
    logging::info("菜单", "点击菜单1，尝试启动nanaGUI");
    try {
        w->show();
    } catch (std::exception a) {
        logging::error("错误", "尝试再启动 Qt GUI 失败");
        logging::error("错误", a.what());
    }
}

CQ_MENU(menu_demo_2) {
    logging::info("菜单", "尝试进入");
}
