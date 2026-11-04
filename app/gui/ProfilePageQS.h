#pragma once

#include "FeaturePageQS.h"

#include <QVBoxLayout>

namespace qs {

/**
 * "我"页面：个人资料（含 T 龄）、关联的其他服务账号与绑定、修改密码、
 * 注销本服务账号（只影响本服务）、聊天节点状态与局域网连接、平台日志、退出登录。
 */
class ProfilePageQS : public FeaturePageQS {
public:
    ProfilePageQS(ShellContextQS& context, QWidget* parent);
    void refresh() override;

private:
    void rebuild();
    QVBoxLayout* layout_;
    QWidget* body_;
};

}  // namespace qs
