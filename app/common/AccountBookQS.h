#pragma once

#include "ServiceAccountQS.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace qs {

/**
 * 账号容器：一个服务内所有账号的集合。
 * 各服务通过"组合"持有一个 AccountBookQS，而不是继承一个"用户管理父类"。
 */
class AccountBookQS {
public:
    bool contains(const std::string& id) const { return accounts_.count(id) != 0; }
    bool isRetired(const std::string& id) const { return retiredIds_.count(id) != 0; }
    void reserveRetired(const std::string& id) { if (!id.empty() && !contains(id)) retiredIds_.insert(id); }
    const ServiceAccountQS* find(const std::string& id) const;
    ServiceAccountQS* find(const std::string& id);
    ResultQS add(ServiceAccountQS account);
    bool remove(const std::string& id);
    std::vector<const ServiceAccountQS*> all() const;
    std::size_t size() const noexcept { return accounts_.size(); }

    /** 昵称/号码模糊查询。 */
    std::vector<const ServiceAccountQS*> search(const std::string& keyword) const;
    /** 找到第一个"链接到 ref"的本服务账号。 */
    const ServiceAccountQS* findLinkedTo(const AccountRefQS& ref) const;
    /** 数字号码分配：返回不小于 start 的、尚未使用的最小号码。 */
    std::string nextNumericId(long long start) const;

    JsonQS toJson() const;
    void fromJson(const JsonQS& j);

private:
    std::map<std::string, ServiceAccountQS> accounts_;
    std::set<std::string> retiredIds_; // prevents surviving cross-service links from changing owners
};

}  // namespace qs
