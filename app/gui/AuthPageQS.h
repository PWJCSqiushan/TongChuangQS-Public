#pragma once

#include "ClientQS.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

#include <functional>
#include <map>
#include <vector>

namespace qs {

/**
 * 某个服务的登录 / 注册页（选择服务之后才出现）。
 * 注册表单字段由服务的 registrationFields() 决定：QQ 号由系统分配、微信可自定义微信号、
 * 微博需要用 QQ 号核验、微商填写微商号——同一个页面，表单随服务变化（多态）。
 * 注册时可勾选"同时开通其他服务"：由平台把请求转给目标服务，目标服务自己完成注册。
 */
class AuthPageQS : public QWidget {
public:
    AuthPageQS(ClientQS& client, std::function<void(const std::string&, const QString&)> onLoggedIn, std::function<void()> onBack,
               QWidget* parent);
    void setService(const std::string& service);
    void setNotice(const QString& text);

private:
    void build();
    void doLogin();
    void doRegister();
    void doConfirmPending();

    ClientQS& client_;
    std::function<void(const std::string&, const QString&)> onLoggedIn_;
    std::function<void()> onBack_;
    std::string service_;
    QWidget* content_ = nullptr;
    QLineEdit* account_ = nullptr;
    QLineEdit* password_ = nullptr;
    QLabel* message_ = nullptr;
    QLabel* notice_ = nullptr;
    std::vector<std::pair<std::string, QLineEdit*>> fields_;
    std::vector<std::pair<std::string, QCheckBox*>> linked_;
};

}  // namespace qs
