#include "AccountBookQS.h"

#include <string>

namespace qs {

const ServiceAccountQS* AccountBookQS::find(const std::string& id) const {
    const auto it = accounts_.find(id);
    return it == accounts_.end() ? nullptr : &it->second;
}

ServiceAccountQS* AccountBookQS::find(const std::string& id) {
    const auto it = accounts_.find(id);
    return it == accounts_.end() ? nullptr : &it->second;
}

ResultQS AccountBookQS::add(ServiceAccountQS account) {
    if (account.id().empty()) {
        return ResultQS::fail("账号不能为空");
    }
    if (contains(account.id()) || retiredIds_.count(account.id())) {
        return ResultQS::fail("账号 " + account.id() + " 已被占用");
    }
    const std::string id = account.id();
    accounts_.emplace(id, std::move(account));
    return ResultQS::ok("账号 " + id + " 创建成功");
}

bool AccountBookQS::remove(const std::string& id) {
    if (!accounts_.erase(id)) return false;
    retiredIds_.insert(id);
    return true;
}

std::vector<const ServiceAccountQS*> AccountBookQS::all() const {
    std::vector<const ServiceAccountQS*> result;
    for (const auto& entry : accounts_) {
        result.push_back(&entry.second);
    }
    return result;
}

std::vector<const ServiceAccountQS*> AccountBookQS::search(const std::string& keyword) const {
    std::vector<const ServiceAccountQS*> result;
    for (const auto& entry : accounts_) {
        if (keyword.empty() || entry.first.find(keyword) != std::string::npos ||
            entry.second.profile().nickname().find(keyword) != std::string::npos) {
            result.push_back(&entry.second);
        }
    }
    return result;
}

const ServiceAccountQS* AccountBookQS::findLinkedTo(const AccountRefQS& ref) const {
    for (const auto& entry : accounts_) {
        if (entry.second.isLinkedTo(ref)) {
            return &entry.second;
        }
    }
    return nullptr;
}

std::string AccountBookQS::nextNumericId(long long start) const {
    long long candidate = start;
    while (contains(std::to_string(candidate)) || retiredIds_.count(std::to_string(candidate))) {
        ++candidate;
    }
    return std::to_string(candidate);
}

JsonQS AccountBookQS::toJson() const {
    JsonQS array = JsonQS::array();
    for (const auto& entry : accounts_) {
        array.push_back(entry.second.toJson());
    }
    return JsonQS{{"accounts", array}, {"retired", retiredIds_}};
}

void AccountBookQS::fromJson(const JsonQS& j) {
    accounts_.clear();
    retiredIds_.clear();
    const auto& list = j.is_array() ? j : j.at("accounts"); // accepts original array stores
    if (j.is_object()) retiredIds_ = j.value("retired", std::set<std::string>());
    if (!list.is_array()) {
        return;
    }
    for (const auto& item : list) {
        ServiceAccountQS account = ServiceAccountQS::fromJson(item);
        if (!account.id().empty()) {
            const std::string id = account.id();
            accounts_.emplace(id, std::move(account));
        }
    }
}

}  // namespace qs
