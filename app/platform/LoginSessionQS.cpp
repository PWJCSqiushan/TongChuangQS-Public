#include "LoginSessionQS.h"

namespace qs {

bool LoginSessionQS::isLoggedIn(const std::string& service) const {
    const auto it = entries_.find(service);
    return it != entries_.end() && it->second.state == SessionStateQS::LoggedIn;
}

bool LoginSessionQS::isPending(const std::string& service) const {
    const auto it = entries_.find(service);
    return it != entries_.end() && it->second.state == SessionStateQS::Pending;
}

std::string LoginSessionQS::accountOf(const std::string& service) const {
    const auto it = entries_.find(service);
    return it == entries_.end() ? std::string() : it->second.account;
}

std::vector<AccountRefQS> LoginSessionQS::loggedInAccounts() const {
    std::vector<AccountRefQS> result;
    for (const auto& e : entries_) {
        if (e.second.state == SessionStateQS::LoggedIn) result.emplace_back(e.first, e.second.account);
    }
    return result;
}

bool LoginSessionQS::anyLoggedIn() const { return !loggedInAccounts().empty(); }

void LoginSessionQS::markLoggedIn(const std::string& service, const std::string& account) {
    entries_[service] = EntryQS{account, SessionStateQS::LoggedIn};
}

void LoginSessionQS::markPending(const std::string& service, const std::string& account) {
    const auto it = entries_.find(service);
    if (it != entries_.end() && it->second.state == SessionStateQS::LoggedIn) {
        return;  // 已登录的服务不降级
    }
    entries_[service] = EntryQS{account, SessionStateQS::Pending};
}

void LoginSessionQS::logout(const std::string& service) { entries_.erase(service); }
void LoginSessionQS::clear() { entries_.clear(); }

}  // namespace qs
