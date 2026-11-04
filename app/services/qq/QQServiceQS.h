#pragma once

#include "FriendBookQS.h"
#include "ServiceBaseQS.h"
#include "StandardFriendsQS.h"
#include "StandardGroupsQS.h"

namespace qs {

/**
 * QQ 服务。号码由系统按顺序分配（5位以上数字），与微博共用号码体系；
 * 支持双向好友、QQ 群（申请加入 + 管理员制度 + 临时讨论组）、点对点 TCP 聊天与群聊。
 */
class QQServiceQS final : public ServiceBaseQS {
public:
    QQServiceQS();

    std::string accentColor() const override { return "#1677FF"; }
    std::string logoText() const override { return "Q"; }
    std::string slogan() const override { return "老朋友都在这里 · 群可申请加入，支持临时讨论组"; }
    std::string accountLabel() const override { return "QQ号"; }
    std::string idRuleText() const override { return "QQ号为5位以上数字，注册时由系统分配；微博与QQ共用号码"; }
    int displayOrder() const override { return 10; }
    std::vector<ServiceFeatureQS> features() const override;

    FriendCapabilityQS* friendCapability() override { return &friends_; }
    const FriendCapabilityQS* friendCapability() const override { return &friends_; }
    GroupCapabilityQS* groupCapability() override { return &groups_; }
    const GroupCapabilityQS* groupCapability() const override { return &groups_; }
    bool supportsChat() const override { return true; }

    bool acceptsLinkedRegistrationFrom(const std::string& fromService) const override;
    std::string linkedRegistrationHint() const override { return "同时开通QQ（系统分配QQ号，密码相同）"; }

protected:
    ResultQS prepareAccountId(const FormQS& form, std::string& id) override;
    void finalizeAccountId(std::string& id) const override;
    ResultQS createLinkedAccount(const UserProfileQS& profile, const std::string& password, const AccountRefQS& source,
                                 std::string& newId) override;
    void onAccountRemoved(const std::string& id) override;
    void loadData() override;
    void saveData() const override;
    void seedDemoData() override;

private:
    FriendBookQS book_;
    StandardFriendsQS friends_;
    StandardGroupsQS groups_;
};

}  // namespace qs
