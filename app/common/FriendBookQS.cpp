#include "FriendBookQS.h"

#include <algorithm>

namespace qs {

bool FriendBookQS::areFriends(const std::string& a, const std::string& b) const {
    const auto it = links_.find(a);
    return it != links_.end() && it->second.count(b) != 0;
}

void FriendBookQS::connect(const std::string& a, const std::string& b, const std::string& date) {
    if (a == b) {
        return;
    }
    links_[a].emplace(b, FriendLinkQS{"", "", date});
    links_[b].emplace(a, FriendLinkQS{"", "", date});
}

bool FriendBookQS::disconnect(const std::string& a, const std::string& b) {
    bool removed = false;
    if (links_.count(a)) removed = links_[a].erase(b) != 0 || removed;
    if (links_.count(b)) removed = links_[b].erase(a) != 0 || removed;
    return removed;
}

std::map<std::string, FriendLinkQS> FriendBookQS::linksOf(const std::string& owner) const {
    const auto it = links_.find(owner);
    return it == links_.end() ? std::map<std::string, FriendLinkQS>() : it->second;
}

FriendLinkQS* FriendBookQS::link(const std::string& owner, const std::string& other) {
    const auto it = links_.find(owner);
    if (it == links_.end()) return nullptr;
    const auto jt = it->second.find(other);
    return jt == it->second.end() ? nullptr : &jt->second;
}

const FriendRequestQS* FriendBookQS::findRequest(const std::string& id) const {
    for (const auto& r : requests_) {
        if (r.id == id) return &r;
    }
    return nullptr;
}

bool FriendBookQS::hasPending(const std::string& from, const std::string& to) const {
    return std::any_of(requests_.begin(), requests_.end(),
                       [&](const FriendRequestQS& r) { return r.from == from && r.to == to; });
}

std::string FriendBookQS::addRequest(const std::string& from, const std::string& to, const std::string& note,
                                     const std::string& date) {
    FriendRequestQS request{"r" + std::to_string(nextRequest_++), from, to, note, date};
    requests_.push_back(request);
    return request.id;
}

void FriendBookQS::removeRequest(const std::string& id) {
    requests_.erase(std::remove_if(requests_.begin(), requests_.end(), [&](const FriendRequestQS& r) { return r.id == id; }),
                    requests_.end());
}

void FriendBookQS::forget(const std::string& account) {
    links_.erase(account);
    for (auto& entry : links_) {
        entry.second.erase(account);
    }
    requests_.erase(std::remove_if(requests_.begin(), requests_.end(),
                                   [&](const FriendRequestQS& r) { return r.from == account || r.to == account; }),
                    requests_.end());
}

JsonQS FriendBookQS::toJson() const {
    JsonQS links = JsonQS::object();
    for (const auto& owner : links_) {
        JsonQS mine = JsonQS::object();
        for (const auto& other : owner.second) {
            mine[other.first] = JsonQS{{"remark", other.second.remark}, {"tag", other.second.tag}, {"since", other.second.since}};
        }
        links[owner.first] = mine;
    }
    JsonQS requests = JsonQS::array();
    for (const auto& r : requests_) requests.push_back(r.toJson());
    return JsonQS{{"links", links}, {"requests", requests}, {"nextRequest", nextRequest_}};
}

void FriendBookQS::fromJson(const JsonQS& j) {
    links_.clear();
    requests_.clear();
    const JsonQS links = j.value("links", JsonQS::object());
    for (const auto& owner : links.items()) {
        for (const auto& other : owner.value().items()) {
            links_[owner.key()][other.key()] =
                FriendLinkQS{other.value().value("remark", std::string()), other.value().value("tag", std::string()),
                             other.value().value("since", std::string())};
        }
    }
    for (const auto& r : j.value("requests", JsonQS::array())) requests_.push_back(FriendRequestQS::fromJson(r));
    nextRequest_ = j.value("nextRequest", 1LL);
}

}  // namespace qs
