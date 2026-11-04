#pragma once

#include "ClientQS.h"
#include <QColor>
#include <QString>
#include <QWidget>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace qs {

class ShellContextQS {
public:
    ClientQS* client = nullptr;
    std::string service;
    QColor accent;
    std::function<void(const std::string& conversationKey)> openConversation;
    std::function<void(const QString& message, bool ok)> toast;
    std::function<void()> refreshAll;
    /** 切换到某个功能页面，并让该页面打开 arg 指定的对象（如联系人账号、群号）。 */
    std::function<void(const std::string& featureKind, const std::string& arg)> navigate;
    /** 退出当前服务的登录（其他服务保持登录）。 */
    std::function<void()> logoutService;
    /** 切换用户：退出全部服务，回到服务选择。 */
    std::function<void()> switchUser;

    ServiceBaseQS& svc() const { return *client->platform().service(service); }
    std::string me() const { return client->accountOf(service); }
    void show(const ResultQS& r) const { toast(QString::fromStdString(r.message()), r.isOk()); }
};

}  // namespace qs
