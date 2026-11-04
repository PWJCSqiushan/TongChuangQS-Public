#pragma once

#include "ServiceBaseQS.h"
#include "StandardGroupsQS.h"
#include "WeiboFollowsQS.h"
#include "WeiboPostQS.h"

#include <vector>

namespace qs {

/**
 * 微博服务。与 QQ 共用号码：用已有 QQ 号开通（经总线请 QQ 服务核验）；
 * 好友 = 互相关注；群为"粉丝群模式"（关注群主即可加入）；
 * 特色功能是"首页"：发微博、看关注的人的动态、点赞。微博没有聊天功能。
 */
class WeiboServiceQS final : public ServiceBaseQS {
public:
    WeiboServiceQS();

    std::string accentColor() const override { return "#FF7A00"; }
    std::string logoText() const override { return "博"; }
    std::string slogan() const override { return "随时随地发现新鲜事 · 与QQ共用号码"; }
    std::string accountLabel() const override { return "微博号"; }
    std::string idRuleText() const override { return "微博与QQ共用号码：使用已有的QQ号开通，号码相同"; }
    int displayOrder() const override { return 30; }
    std::vector<ServiceFeatureQS> features() const override;

    FriendCapabilityQS* friendCapability() override { return &follows_; }
    const FriendCapabilityQS* friendCapability() const override { return &follows_; }
    GroupCapabilityQS* groupCapability() override { return &groups_; }
    const GroupCapabilityQS* groupCapability() const override { return &groups_; }

    bool acceptsLinkedRegistrationFrom(const std::string& fromService) const override { return fromService == "qq"; }
    std::string linkedRegistrationHint() const override { return "同时开通微博（与QQ号相同）"; }

    // ———— 微博特色功能 ————
    ResultQS post(const std::string& me, const std::string& text);
    ResultQS toggleLike(const std::string& me, const std::string& postId);
    ResultQS deletePost(const std::string& me, const std::string& postId);
    /** 首页时间线：自己和关注的人的微博，按时间倒序。 */
    std::vector<WeiboPostQS> timeline(const std::string& me) const;
    std::vector<WeiboPostQS> postsOf(const std::string& author) const;
    const WeiboFollowsQS& follows() const { return follows_; }

protected:
    std::vector<RegistrationFieldQS> idFields() const override;
    ResultQS prepareAccountId(const FormQS& form, std::string& id) override;
    void onAccountCreated(ServiceAccountQS& account, const FormQS& form) override;
    ResultQS createLinkedAccount(const UserProfileQS& profile, const std::string& password, const AccountRefQS& source,
                                 std::string& newId) override;
    void onAccountRemoved(const std::string& id) override;
    void loadData() override;
    void saveData() const override;
    void seedDemoData() override;

private:
    WeiboFollowsQS follows_;
    StandardGroupsQS groups_;
    std::vector<WeiboPostQS> posts_;
    long long nextPost_ = 1;
};

}  // namespace qs
