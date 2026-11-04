#pragma once

#include "JsonQS.h"

#include <string>
#include <tuple>

namespace qs {

/**
 * "某服务中的某个账号"的引用，例如 {qq, 10001}、{wechat, qiushan}。
 * 平台与服务之间、服务与服务之间传递身份时只传这个值对象，
 * 不传任何服务内部的数据结构——这正是"服务彼此不透明"的体现。
 */
class AccountRefQS {
public:
    AccountRefQS() = default;
    AccountRefQS(std::string service, std::string account)
        : service_(std::move(service)), account_(std::move(account)) {}

    const std::string& service() const noexcept { return service_; }
    const std::string& account() const noexcept { return account_; }
    bool empty() const noexcept { return service_.empty() || account_.empty(); }
    std::string key() const { return service_ + ":" + account_; }

    JsonQS toJson() const { return JsonQS{{"service", service_}, {"account", account_}}; }
    static AccountRefQS fromJson(const JsonQS& j) {
        return AccountRefQS(j.value("service", std::string()), j.value("account", std::string()));
    }

    friend bool operator==(const AccountRefQS& a, const AccountRefQS& b) {
        return a.service_ == b.service_ && a.account_ == b.account_;
    }
    friend bool operator!=(const AccountRefQS& a, const AccountRefQS& b) { return !(a == b); }
    friend bool operator<(const AccountRefQS& a, const AccountRefQS& b) {
        return std::tie(a.service_, a.account_) < std::tie(b.service_, b.account_);
    }

private:
    std::string service_;
    std::string account_;
};

}  // namespace qs
