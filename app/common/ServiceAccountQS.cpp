#include "ServiceAccountQS.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>

namespace qs {

ServiceAccountQS::ServiceAccountQS(std::string id, const std::string& password, UserProfileQS profile)
    : id_(std::move(id)), passwordDigest_(digest(password)), profile_(std::move(profile)) {}

std::string ServiceAccountQS::digest(const std::string& password) {
    std::uint64_t hash = 1469598103934665603ULL;
    const std::string salted = "TongChuangQS#" + password;
    for (unsigned char c : salted) {
        hash ^= c;
        hash *= 1099511628211ULL;
    }
    char buffer[24];
    std::snprintf(buffer, sizeof buffer, "%016llx", static_cast<unsigned long long>(hash));
    return buffer;
}

bool ServiceAccountQS::checkPassword(const std::string& password) const { return digest(password) == passwordDigest_; }
void ServiceAccountQS::setPassword(const std::string& password) { passwordDigest_ = digest(password); }

bool ServiceAccountQS::isLinkedTo(const AccountRefQS& ref) const {
    return std::find(links_.begin(), links_.end(), ref) != links_.end();
}

void ServiceAccountQS::addLink(const AccountRefQS& ref) {
    if (!ref.empty() && !isLinkedTo(ref)) {
        links_.push_back(ref);
    }
}

void ServiceAccountQS::removeLink(const AccountRefQS& ref) {
    links_.erase(std::remove(links_.begin(), links_.end(), ref), links_.end());
}

JsonQS ServiceAccountQS::toJson() const {
    JsonQS links = JsonQS::array();
    for (const auto& link : links_) {
        links.push_back(link.toJson());
    }
    return JsonQS{{"id", id_}, {"password", passwordDigest_}, {"profile", profile_.toJson()}, {"links", links}};
}

ServiceAccountQS ServiceAccountQS::fromJson(const JsonQS& j) {
    ServiceAccountQS account;
    account.id_ = j.value("id", std::string());
    account.passwordDigest_ = j.value("password", std::string());
    account.profile_ = UserProfileQS::fromJson(j.value("profile", JsonQS::object()));
    for (const auto& link : j.value("links", JsonQS::array())) {
        account.links_.push_back(AccountRefQS::fromJson(link));
    }
    return account;
}

}  // namespace qs
