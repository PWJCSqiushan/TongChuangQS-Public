#pragma once

#include "FriendBookQS.h"
#include "ServiceBaseQS.h"
#include "StandardFriendsQS.h"
#include "StandardGroupsQS.h"
#include "WeShopProductQS.h"

#include <vector>

namespace qs {

/**
 * 微商服务——"可扩展性"演示用的新服务。
 *
 * 整个服务只由本目录（services/weshop）中的文件组成：继承 ServiceBaseQS、
 * 组合复用好友/群组件、在 .cpp 末尾自注册。平台层、界面框架、其他服务
 * 均未因它修改过一行代码；把本目录从 CMake 的服务目录中移走，平台照常运行，只是少一个选项。
 *
 * 号码独立（可绑定微信号）；好友称为"客户"；社群沿用微信群模式；特色功能是"小店"。
 */
class WeShopServiceQS final : public ServiceBaseQS {
public:
    WeShopServiceQS();

    std::string accentColor() const override { return "#7C4DFF"; }
    std::string logoText() const override { return "商"; }
    std::string slogan() const override { return "朋友圈里的小店 · 新增服务，平台零修改"; }
    std::string accountLabel() const override { return "微商号"; }
    std::string idRuleText() const override { return "微商号独立设置：字母开头，4-20位；可绑定微信号"; }
    int displayOrder() const override { return 40; }
    std::vector<ServiceFeatureQS> features() const override;

    FriendCapabilityQS* friendCapability() override { return &customers_; }
    const FriendCapabilityQS* friendCapability() const override { return &customers_; }
    GroupCapabilityQS* groupCapability() override { return &groups_; }
    const GroupCapabilityQS* groupCapability() const override { return &groups_; }

    bool acceptsLinkedRegistrationFrom(const std::string& fromService) const override { return fromService == "wechat"; }
    std::string linkedRegistrationHint() const override { return "同时开通微商小店（自动绑定微信）"; }
    std::vector<std::string> bindableServices() const override { return {"wechat"}; }

    // ———— 小店 ————
    ResultQS listProduct(const std::string& me, const std::string& name, const std::string& description, int priceCents,
                         int stock);
    ResultQS removeProduct(const std::string& me, const std::string& productId);
    ResultQS buy(const std::string& me, const std::string& productId, int quantity);
    std::vector<WeShopProductQS> productsOf(const std::string& owner) const;
    /** 客户（好友）的小店商品。 */
    std::vector<WeShopProductQS> customerProducts(const std::string& me) const;
    std::vector<WeShopOrderQS> ordersOf(const std::string& me) const;

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
    StandardFriendsQS customers_;
    StandardGroupsQS groups_;
    std::vector<WeShopProductQS> products_;
    std::vector<WeShopOrderQS> orders_;
    long long nextId_ = 1;
};

}  // namespace qs
