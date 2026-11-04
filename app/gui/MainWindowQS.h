#pragma once

#include "ClientQS.h"

#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>

#include <map>

namespace qs {

class AuthPageQS;
class ServiceSelectPageQS;
class ServiceShellQS;

/**
 * 主窗口：在"选择服务 → 登录/注册 → 服务主界面"之间切换；
 * 每 60 毫秒驱动一次 ClientQS::pump()（处理网络消息、心跳补发、读取其他窗口的数据修改），
 * 并把聊天事件分发给各服务的界面。
 */
class MainWindowQS : public QMainWindow {
public:
    MainWindowQS(ClientQS& client, const QString& windowTag);
private:
    friend class SnapshotTourQS;  // 截图脚本（开发辅助，见 SnapshotTourQS.cpp）
    friend class ChatSelfTestQS;  // 双进程聊天自检（开发辅助）
    void showSelect();
    void pickService(const std::string& service);
    void enterShell(const std::string& service, const QString& message);
    void logoutService(const std::string& service);
    void switchUser();
    void pump();
    void updateTitle();

    ClientQS& client_;
    QString tag_;
    QStackedWidget* stack_;
    ServiceSelectPageQS* select_;
    AuthPageQS* auth_;
    std::map<std::string, ServiceShellQS*> shells_;
    QTimer timer_;
    int ticks_ = 0;
};

}  // namespace qs
