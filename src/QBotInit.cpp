#include <iostream>
#include <set>
#include <sstream>
#include <exception>
#include <mutex>

#include <CThreadPool.hpp>
#include <QApplication>
#include <QMessageBox>

#include "Global.hpp"
#include "PatternController.hpp"
#include "QBotUtils.hpp"
#include "mainwindow.h"

using namespace cq;
using namespace std;
using Message = cq::message::Message;
using MessageSegment = cq::message::MessageSegment;

#ifdef TYR_ACC_STD_STREAM
static const auto _ = []() {
    ios::sync_with_stdio(false);
    // cin.tie(nullptr);
    return nullptr;
}();
#endif

PatternController *pc_instance = nullptr;
QApplication *a = nullptr;
MainWindow *w = nullptr;
int tmp_num = 0;
bool is_ready = false;
char **tmp_argv = new char*[0];

std::mutex gui_des_lock;

nThread::CThreadPool tp{MAX_AVALIBEL_THREAD};

void private_message_handler (const PrivateMessageEvent &event){
    try {
        std::vector<std::string> params;
        QBot_Utils::split_private_msg(event.message, Pattern::param_splitor, params);
        if (params.empty()) {
            send_message(event.target, Pattern::invalid_param_reply);
            return;
        }
        auto pattern = pc_instance->find_pattern(params[0]);
        if (pattern == pc_instance->get_end() || !pattern->second.is_enable) {
            send_message(event.target, Pattern::invalid_param_reply);
        } else {
            const auto& reply_pattern = pattern->second;
            std::string reply;
            reply_pattern.get_reply_msg(params, reply);
            #ifdef DEBUG_VERSION
            logging::debug("尝试发送消息内容", reply);
            #endif
            send_message(event.target, reply);
        }
    } catch (ApiError &err) {
        logging::error("系统错误", "私聊消息处理错误, 错误码: " + to_string(err.code));
        if (!Pattern::i_need_help.empty()) {
            try {
                send_message(event.target, Pattern::i_need_help);
            } catch (ApiError &err) {
                logging::error("系统错误", "太惨了，呼救都发不出去, 错误码: " + to_string(err.code));
            }
        }
    } catch (std::exception &err) {
        logging::error("私聊消息发送错误", err.what());
        if (!Pattern::i_need_help.empty()) {
            try {
                send_message(event.target, Pattern::i_need_help);
            } catch (ApiError &err) {
                logging::error("系统错误", "太惨了，呼救都发不出去, 错误码: " + to_string(err.code));
            }
    }
}}

void group_message_handler(const GroupMessageEvent &event){
    try {
        if (!QBot_Utils::is_mentioned(event.message)) {
            #ifdef DEBUG_VERSION
            logging::debug("group_missing_msg", event.message);
            #endif
            return;
        } else {
            std::vector<std::string> params;
            QBot_Utils::split_group_msg(event.message, Pattern::param_splitor, params);
            if (params.empty()) {
                send_group_message(event.group_id, Pattern::invalid_param_reply);
                return;
            }
            auto pattern = pc_instance->find_pattern(params[0]);
            if (pattern == pc_instance->get_end() || !pattern->second.is_enable) {
                if (!event.is_anonymous()) {
                    send_group_message(event.group_id, QBot_Utils::get_mention_key(to_string(event.user_id)) + Pattern::invalid_param_reply);
                } else {
                    send_group_message(event.group_id, Pattern::invalid_param_reply);
                }
            } else {
                std::string reply;
                pattern->second.get_reply_msg(params, reply);
                #ifdef DEBUG_VERSION
                logging::debug("尝试发送消息内容", reply);
                #endif
                if (!event.is_anonymous()) {
                    send_group_message(event.group_id, QBot_Utils::get_mention_key(to_string(event.user_id)) + reply);
                }
                else {
                    send_group_message(event.group_id, reply);
                }      
            }
        }
    } catch (ApiError &err) {
        logging::error("系统错误", "群聊消息处理错误, 错误码: " + to_string(err.code));
        if (!Pattern::i_need_help.empty()) {
            try {
                std::string reply;
                if (!event.is_anonymous()) {
                    reply = QBot_Utils::get_mention_key(to_string(event.user_id));
                }
                send_message(event.target, reply + Pattern::i_need_help);
            } catch (ApiError &err) {
                logging::error("系统错误", "太惨了，呼救都发不出去, 错误码: " + to_string(err.code));
            }
        }
    } catch (std::exception &err) {
        logging::error("群聊消息发送错误", err.what());
        if (!Pattern::i_need_help.empty()) {
            try {
                std::string reply;
                if (!event.is_anonymous()) {
                    reply = QBot_Utils::get_mention_key(to_string(event.user_id));
                }
                send_message(event.target, reply + Pattern::i_need_help);
            } catch (ApiError &err) {
                logging::error("系统错误", "太惨了，呼救都发不出去, 错误码: " + to_string(err.code));
            }
        }
}}

CQ_INIT {
    on_enable([] { 
        #ifdef DEBUG_VERSION
        logging::info("启动", "WoWsQBot插件已启用【您正在使用测试版】");
        logging::info("提示", "测试版的性能和稳定性均不能保证，请慎重使用");
        #endif

        logging::info("提示", "本插件 Github 地址：https://github.com/SiskonEmilia/WoWs-QBotPlugin");
        logging::info("提示", "如有发生严重 BUG，请在 Github 页面提交 issue");
        try {
            if (a == nullptr) a = new QApplication(tmp_num, tmp_argv);
            if (w == nullptr) w = new MainWindow();
            w->show();
            a->exec();
        } catch(const std::exception &err) {
            logging::error("错误", "GUI框架初始化时发生错误");
            logging::error("错误", err.what());
        }

        #ifdef DEBUG_VERSION
        logging::debug("测试", "测试：文件读取");
        try {
            pc_instance = PatternController::get_instance();
            pc_instance->load_from_file();
            logging::debug("测试", "测试：文件读取成功");
            pc_instance->save_to_file();
            logging::debug("测试", "测试：文件保存成功");
        }
        catch(const std::exception& e) {
            logging::error("错误", "文件存取时发生错误");
            logging::error("错误", e.what());
        }
        #else
        try {
            pc_instance = PatternController::get_instance();
            pc_instance->load_from_file();
        }
        catch(const std::exception& e) {
            logging::error("错误", "文件读取时发生错误");
            logging::error("错误", e.what());
        }
        #endif

        QBot_Utils::set_mentioned_key(std::to_string(cq::get_login_user_id()));
        is_ready = true;
        #ifdef DEBUG_VERSION
        logging::debug("mentioned_key", Pattern::bot_mentioned_key);
        #endif
    });

    on_private_message([](const PrivateMessageEvent &event) {
        event.block();
        if (!is_ready) return;
        tp.add_and_detach(private_message_handler, PrivateMessageEvent(event));
    });

    
    #ifdef DEBUG_VERSION
    on_message([](const MessageEvent &event) {
        logging::debug("消息", "收到消息: " + event.message + "\n实际类型: " + typeid(event).name());
    });
    #endif

    on_group_message([](const GroupMessageEvent &event) {
        event.block();
        if (!is_ready) return;
        tp.add_and_detach(group_message_handler, GroupMessageEvent(event));
    });

    // on_discuss_message([](const DiscussMessageEvent& event) {
    //     try {
    //         if (!QBot_Utils::is_mentioned(event.message)) {
    //             #ifdef DEBUG_VERSION
    //             logging::debug("discuss_missing_msg", event.message);
    //             #endif
    //             event.block();
    //             return;
    //         } else {
    //             std::vector<std::string> params;
    //             QBot_Utils::split_group_msg(event.message, Pattern::param_splitor, params);
    //             if (params.empty()) {
    //                 send_group_message(event.discuss_id, Pattern::invalid_param_reply);
    //                 event.block();
    //                 return;
    //             }
    //             auto pattern = pc_instance->find_pattern(params[0]);
    //             if (pattern == pc_instance->get_end() || !pattern->second->is_enable) {
    //                 send_discuss_message(event.discuss_id, Pattern::invalid_param_reply);
    //             } else {
    //                 std::string reply;
    //                 reply = QBot_Utils::get_mention_key(to_string(event.user_id));
                    
    //                 pattern->second->get_reply_msg(params, reply);
    //                 #ifdef DEBUG_VERSION
    //                 logging::debug("尝试发送消息内容", reply);
    //                 #endif
    //                 send_discuss_message(event.discuss_id, reply);
    //             }
    //         }
    //     } catch (ApiError &err) {
    //         logging::error("系统错误", "群聊消息处理错误, 错误码: " + to_string(err.code));
    //     } catch (std::exception &err) {
    //         logging::error("群聊消息发送错误", err.what());
    //     }
    //     event.block(); // 阻止当前事件传递到下一个插件
    // });

    on_group_request([](const GroupRequestEvent &event){ // 群请求
        if (!is_ready) return;
        if (event.sub_type == GroupRequestEvent::SubType::INVITE) {
            set_group_request(event.flag, event.sub_type, RequestEvent::Operation::APPROVE);
            logging::info("群邀请", "已自动同意一个加群邀请");
        }
    });

    
    // #ifdef DEBUG_VERSION
    // on_group_upload([](const auto &event) { // 可以使用 auto 自动推断类型
    //     stringstream ss;
    //     ss << "您上传了一个文件, 文件名: " << event.file.name << ", 大小(字节): " << event.file.size;
    //     try {
    //         send_message(event.target, ss.str());
    //     } catch (ApiError &) {
    //     }
    // });
    // #endif

    on_disable([]() {
        gui_des_lock.lock();
        if (w != nullptr) {
            delete w;
            w = nullptr;
        }

        if (a != nullptr) {
            delete a;
            a = nullptr;
        }
        gui_des_lock.unlock();
    });

    on_coolq_exit([]() {
        gui_des_lock.lock();
        if (w != nullptr) {
            delete w;
            w = nullptr;
        }
        if (a != nullptr) {
            delete a;
            a = nullptr;
        }
        gui_des_lock.unlock();
    });
}

CQ_MENU(menu_config) {
    #ifdef GUI_TEST_SET
    try {
        w->show();
    } catch (std::exception a) {
        logging::error("错误", "尝试启动 Qt GUI 失败");
        logging::error("错误", a.what());
    }
    #else
        QMessageBox::information(NULL, "提示", "暂不支持图形化界面。");
    #endif
}

CQ_MENU(menu_reload_from_file) {
    pc_instance->reset();
    try {    
        pc_instance->load_from_file();
        QMessageBox::information(NULL, "提示", "已成功加载文件内设置，原有设置被清除。");
    } catch (const std::exception &err) {
        logging::error("文件加载错误", err.what());
        QMessageBox::critical(NULL, "发生错误", "从文件加载时发生错误，请查看应用日志。");
    }
}
