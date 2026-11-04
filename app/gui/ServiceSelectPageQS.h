#pragma once

#include "ClientQS.h"

#include <QGridLayout>
#include <QLabel>
#include <QWidget>

#include <functional>

namespace qs {

/**
 * 启动后的第一个页面：选择服务（老师要求"主界面先选服务，再登录"）。
 * 服务卡片由平台的服务列表动态生成——新增服务自动出现一张卡片。
 * 卡片显示状态：已登录 / 已开通·一键进入（其他服务登录后自动发现）/ 未登录。
 */
class ServiceSelectPageQS : public QWidget {
public:
    using PickQS = std::function<void(const std::string& service)>;
    ServiceSelectPageQS(ClientQS& client, PickQS onPick, std::function<void()> onSwitchUser, QWidget* parent);
    void refresh();

private:
    ClientQS& client_;
    PickQS onPick_;
    QGridLayout* grid_;
    QLabel* identity_;
};

}  // namespace qs
