#pragma once

#include "FeaturePageQS.h"

#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace qs {

/**
 * "联系人"页面：只依赖 FriendCapabilityQS 接口，所以 QQ/微信（申请-同意）、微博（关注-回关）、
 * 微商（客户）共用同一个页面，显示的文字（"好友""关注""客户"）由能力对象提供。
 * 功能：好友列表（按分组）、查询、新的朋友（申请处理）、按号码添加、
 *       从其他服务推荐好友（经平台总线）、共同好友、修改备注与分组、删除、发消息。
 */
class ContactsPageQS : public FeaturePageQS {
public:
    ContactsPageQS(ShellContextQS& context, QWidget* parent);

    void refresh() override;
    int badgeCount() const override;
    void activate(const std::string& account) override;
    void refreshPresence() override { rebuildList(); }

private:
    void rebuildList();
    void showFriend(const std::string& account);
    void showRequests();
    void showAddPanel();
    void clearDetail();
    void updateSelection();
    QWidget* card(QWidget* parent, QVBoxLayout*& inner);

    QLineEdit* search_;
    QListWidget* list_;
    QWidget* detail_;
    QVBoxLayout* detailLayout_;
    std::string selected_;  // 账号，或 "@requests" / "@add"
};

}  // namespace qs
