#pragma once

#include "AccountRefQS.h"
#include "UserProfileQS.h"

#include <string>
#include <vector>

namespace qs {

/**
 * 某一个服务内部的一条账号记录：号码 + 密码摘要 + 基本信息 + 与其他服务账号的绑定关系。
 * 每个服务各自保存自己的账号记录，互不共享（老师强调的"数据按服务分开存储"）。
 */
class ServiceAccountQS {
public:
    ServiceAccountQS() = default;
    ServiceAccountQS(std::string id, const std::string& password, UserProfileQS profile);

    const std::string& id() const noexcept { return id_; }
    const UserProfileQS& profile() const noexcept { return profile_; }
    UserProfileQS& profile() noexcept { return profile_; }

    bool checkPassword(const std::string& password) const;
    void setPassword(const std::string& password);

    const std::vector<AccountRefQS>& links() const noexcept { return links_; }
    bool isLinkedTo(const AccountRefQS& ref) const;
    void addLink(const AccountRefQS& ref);
    void removeLink(const AccountRefQS& ref);

    JsonQS toJson() const;
    static ServiceAccountQS fromJson(const JsonQS& j);

    /** 演示用摘要算法（FNV-1a），避免明文保存密码；不是生产级加密。 */
    static std::string digest(const std::string& password);

private:
    std::string id_;
    std::string passwordDigest_;
    UserProfileQS profile_;
    std::vector<AccountRefQS> links_;
};

}  // namespace qs
