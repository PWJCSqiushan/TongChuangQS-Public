#pragma once

#include "FriendBookQS.h"
#include "ServiceBaseQS.h"
#include "StandardFriendsQS.h"
#include "StandardGroupsQS.h"

namespace qs {

/**
 * 微信服务。微信号独立于 QQ（可自定义，留空则系统分配 wxid_xxx），可以绑定 QQ 号；
 * 群为"微信群模式"：只能邀请加入、仅群主有特权、没有讨论组。
 * 联系人页面支持"从QQ好友推荐"（平台经总线向 QQ 服务查询）。
 */
class WeChatServiceQS final : public ServiceBaseQS {
public:
    WeChatServiceQS();

    std::string accentColor() const override { return "#07C160"; }
    std::string logoText() const override { return "微"; }
    std::string slogan() const override { return "熟人社交 · 群只能邀请加入，可添加QQ好友"; }
    std::string accountLabel() const override { return "微信号"; }
    std::string idRuleText() const override { return "微信号独立设置：字母开头，6-20位字母、数字或下划线；可绑定QQ号"; }
    int displayOrder() const override { return 20; }
    std::vector<ServiceFeatureQS> features() const override;

    FriendCapabilityQS* friendCapability() override { return &friends_; }
    const FriendCapabilityQS* friendCapability() const override { return &friends_; }
    GroupCapabilityQS* groupCapability() override { return &groups_; }
    const GroupCapabilityQS* groupCapability() const override { return &groups_; }
    bool supportsChat() const override { return true; }

    bool acceptsLinkedRegistrationFrom(const std::string& fromService) const override;
    std::string linkedRegistrationHint() const override { return "同时开通微信（系统分配微信号，自动绑定）"; }
    std::vector<std::string> bindableServices() const override { return {"qq"}; }

    static bool isValidWechatId(const std::string& id);
    static std::string generateWechatId();

protected:
    std::vector<RegistrationFieldQS> idFields() const override;
    ResultQS prepareAccountId(const FormQS& form, std::string& id) override;
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
